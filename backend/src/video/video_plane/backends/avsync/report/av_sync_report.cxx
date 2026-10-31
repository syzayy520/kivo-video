#include "video/video_plane/backends/avsync/report/av_sync_report.hpp"

#include "video/video_plane/backends/mediacodec/device/mediacodec_json_string_escaper.hpp"
#include "video/video_plane/core/audio/pcm_format.hpp"
#include "video/video_plane/core/pace/sampling_mode.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace kivo::video::avsync {

// 半程一致性（P13-O）住 `core::pace`：它是纯判定，与报告层无关。
// 起个短别名而不是 using namespace —— 这个文件已经同时碰 avsync / mediacodec /
// core 三棵子树，展开整棵命名空间会让"这个名字属于哪一层"看不见。
namespace pace = kivo::video::core::pace;

namespace {

/// 「这个分布没有位置可言」的显式常量。
///
/// 与"有位置但还没测到"（`valid == false` 但将来会有）在 JSON 里表现相同 ——
/// 都是不写 `worst` 键。区别只在注释里：这里是**结构上就没有**媒体时刻，
/// 不是漏采。
const core::pace::DriftExtreme kNoExtreme{};

using core::pace::SignedHistogram;

/// 版本历史：
///   /1（C8，首版）：把 C1–C7 的事实收进一份文件。
///   /2（P13-D）：`av_drift` 的分位数改由**按媒体时间抽稀到 20 Hz** 的分布给出，
///       并新增 `sampling` / `per_frame` / `trend` 三块。
///       原因：逐帧采样的 p99 随帧率变（60fps 的样本密度是 30fps 的两倍），
///       同一台机器读出"60fps 更差"，差的是看得更清楚。
///       `/1` 的证据不重写（重写等于伪造），校验器同时接受两个版本。
///   /3（P13-E）：`av_drift` 下**不再有任何裸分位数**，改挂到 `quantiles.<口径>`；
///       `sampling` 增加 `cross_frame_rate_set` 指向唯一可比的那一份（或 null）。
///       原因：/2 虽然把口径写进了 `sampling.mode`，但 `av_drift.p99` 这个键名本身
///       不带口径 —— 复制一个数去填表格的人，永远想不起去看三行之外。
///       "记得看注释"不是保障，"结构上取不出裸数"才是。
///   /4（P13-F）：`quantiles.<口径>` 新增 `worst`（`index` / `of` / `media_ns` / `lateness_ns`）。
///       原因：`max` 只说最差**多大**，说不出最差**在哪儿** —— 而"片头冷启动"与
///       "片尾收尾"对结论的影响完全相反。4K60 r1 那个 88 ms 因此无法归因
///       （见 `65-p13e-status.md` §六.1）。位置是**序列**的属性，直方图按定义没有，
///       所以由 `DriftSeries` 累计维护后上报。
///   /5（P13-G）：抽稀块有了**结构门**（`decimated_block_is_usable`：≥2 样本且跨度 >0）。
///       原因：1 个样本的 `p50/p95/p99/max` 四个数完全相同，长得像一份正常数据，
///       E1/E2/E3/F1/F2′ 全套判据都会放它过去，而缺 pts 的流正好产出这种形态。
///       降级时 `sampling.reason` 由 `decimated_unusable_reason()` 唯一给出。
///       —— `/5` 当时漏了这条历史记录，P13-H 补上（schema 号动了、说明没动，
///       与 P13-E 那次 `card_checks` 漏写同族）。
///   /6（P13-H）：`sampling` 新增 `media_span_ns`（**整场**媒体时间跨度），
///       `observed_hz` 以它为分母。原因：`accepted` 是累计的，分母却取了
///       `trend.span_ns`（**窗口**跨度，封顶 102.35 s），于是超过 102.4 s 之后
///       `observed_hz` 随片长线性增长（130 s → 25.4 Hz，600 s → 117 Hz），
///       而旁边写着 `target_hz: 20`。那个数没有上界，也没有判据拦它 ——
///       校验器复算的是同一个除法（见 `70-p13h-design.md` §一）。
///   /7（P13-I）：`trend` 自带 `capacity` / `window_sample_count` / `segment_accepted`
///       与 `window_quantiles`（**窗口**分位数 + 分布）；删掉含糊的 `segment_samples`。
///       两件事一起改的：
///       ① `truncated` 上一版写的是"整场累计 > 容量"，seek 之后会谎报覆盖 ——
///          覆盖是**每段**的事，所以段内接受数必须单独算、单独报；
///       ② `quantiles.<口径>` 是整场累计、`trend` 是最近 102.4 s，两个数在长片源上
///          不是同一批样本，却并排摆着让人当同一批读。趋势自带分布之后，
///          "这段抖多少"和"这段漂多少"同源，不必跨块借数（`72-p13i-design.md` §二）。
///       `segment_samples` 这个名字同时被读成"这段采了几个"和"窗口里剩几个"，
///       一删为二：`segment_accepted`（段口径）与 `window_sample_count`（窗口口径）。
///   /8（P13-J）：给斜率配误差棒 —— `slope_ppm_std_error` / `slope_ci_low_ppm` /
///       `slope_ci_high_ppm` / `slope_detectable`，外加窗口中位数是否精确的
///       `window_median_exact` / `window_median_fallback_windows`。
///       起因是两件事同时为真，而报告只让读者看见其中一件：
///       ① 八个窗口中位数**全部等于 125000**（三轮真机无例外）—— 0.25 ms 桶中心
///          把 29 µs~250 µs 那段真实差异一口吃掉，`delta_median_ns` 恒 0、
///          `slope_from_windows_ppm` 恒 0。那不是"没有漂移"，是"尺子上只有一个刻度"；
///       ② `slope_ppm` 在 `r2 = 5.5e-05` 之下照样印六位小数，没有误差棒，
///          于是纯噪声被读成"精确测到 −0.022 ppm"。
///       顺带纠正一句从 P13-D 说到 P13-I 的话：漂移量准**不是**卡在 7 ppm。
///       逐点回归的标准误实测只有 ~0.075 ppm，分辨力比 7 ppm 好一个半数量级 ——
///       缺的是刻度盘，不是尺子（见 `74-p13j-design.md` §一）。
///   /9（P13-K）：`slope_ppm_std_error` **换了模型**（名字没变、语义变了），
///       并新增 `slope_ppm_std_error_iid` / `slope_ppm_std_error_inflation` /
///       `hac_lags` / `hac_lag1_autocorrelation` / `hac_estimated`，
///       以及半程诊断 `slope_first_half_ppm` / `slope_second_half_ppm` /
///       `slope_halves_agree`。
///   /13（P13-O）：半程各自带 HAC 标准误与 95% 带，新增 `slope_halves_*`
///       共 12 格与三态 `slope_halves_relation`；`slope_halves_agree` 降为历史字段。
///       判据：`inconsistent ⇒ drift_verdict == unknown`。
///       起因是 `/8` 自己造出的一次假阳性：r3（35 s / 700 点）报
///       `slope_detectable = true`，而那条线 `r2 = 0.007` —— 只解释 0.7% 方差。
///       `/8` 的 SE 假设残差独立同分布，而 `lateness` 的相邻样本正相关
///       （vsync 相位、调频、抽稀本身就抹平了慢变分量），有效样本数远小于 `n`，
///       SE 被系统性低估、CI 过窄 —— 于是"看不见的"被判成"看得见"。
///       换成 Newey–West（Bartlett 核）之后同一份数据给出更宽的带。
///       为什么**改名而不并排**：读者不该在 `_iid` 与 `_hac` 之间选一个信，
///       该用的那个必须就叫 `slope_ppm_std_error`；旧值留 `_iid` 只为对照。
///       这个决定本身有风险（同名不同义是这份证据最忌讳的病），
///       所以它被写进状态卡诚实清单第一条，并由判据 K0 盯住三个字段必须同时存在。
///   /10（P13-L）：`slope_detectable` 后面接上 `drift_accumulated_ns` /
///       `drift_accumulated_upper_ns` / `drift_budget_ns` / `drift_verdict`。
///       起因是一句被读错了两张卡的话：`detectable = true` 被当成"漂移要紧"。
///       它只说"与 0 分得开"。P13-K 的 r3 报着 `detectable = true`，
///       对应的斜率 0.48 ppm 在 35 秒里只累积 **17 µs** —— 比一次 vsync 抖动还小。
///       分得开是因为尺子细，不要紧是因为量太小，两件事必须分开说。
///       反方向的漏报同样真实：CI 含 0（`detectable = false`）时，CI 远端
///       积起来也可能越过人耳阈值 —— 所以判定用的是**上界**，不是点估计。
///       `drift_verdict` 取三态而非布尔：`unknown`（这份趋势没资格下结论）
///       与 `within_budget`（有资格，且没过）印成同一个 `false`，
///       就是本项目反复栽的那种"让失败伪装成通过"。
///   /11（P13-M）：`segments` 后面接上 `segment_largest_gap_cells` /
///       `segment_contiguous`。
///       起因是 P13-L 撞出的一份真机缺陷证据：在 30 s 处向前跳到 40 s，
///       报告照样写 `segments = 1`、`span = 112.367 s`、`observed_hz = 18.466`，
///       而实际是两段之间隔着十秒空白。
///       根因不是"阈值不够"，是**判定用错了信号**：上一版拿
///       `media_ns < last_media_ns_` 推断"换了时间轴"。pts 倒退确实是换了
///       时间轴（充分条件），但 pts 前进推不出"没换"—— 向前 seek 正是
///       "前进且换了"。用存在性推断不存在性，必然漏一次。
///       现在段边界由 `FramePacer::reset()`（锚点重建）决定，精确、无阈值；
///       段内真空洞（卡顿）无法与向前 seek 在 pts 上区分，因此**不切段**，
///       只出版"最宽的那个洞有多宽"，让读者知道 `span_ns` 里含着一个洞。
///   /12（P13-N）：每个 `quantiles.<口径>` 的 `p50` 后面接上
///       `p50_range_low_ns` / `p50_range_high_ns` / `p50_bucket_samples` /
///       `bucket_width_ns`；`trend` 里接上 `median_of_window_medians_ns`。
///       起因是同一份证据里的两个 p50 相差 66%，而 JSON 里没有任何字段
///       能阻止读者把它读成"两份测量打架"：
///       `quantiles.media_time_decimated.p50` 是直方图桶中心（恒 125 000），
///       `first/last_window_median_ns` 是原始值选出的真值（74 835）。
///       它们不打架 —— 真值确实在 [0, 250000) 那一桶里，是桶中心没本事说出来。
///       所以修法不是把 125000 改成 "124999"（那不是更准，是假装更准），
///       而是**让 p50 自己说出它可能的范围**，并给整批点一个精确参照。
///       这个错从 P13-J 就存在：那轮把窗口中位数换成精确值，
///       却没管同名的整场 p50 还是桶中心，于是同一份 JSON 里出现了
///       两根刻度不同的尺子，而它们看起来是同一把。
///
///   /21（P14-E）：`audio.clock.trace` —— 异常前后 16 个原始观测的窗口。
///       计数说不出"设备跳了"还是"我们算错了"，序列可以。
///   /22（P14-G）：`audio.clock.tail` —— 每一次观测的（设备步长 − 墙钟步长）
///       按量级归档。82 分钟里只出现了 1 次钳制，而"多久一次"这个问题的
///       泊松区间是 [0.03, 5.6] —— 一个事件回答不了频率。尾部把问题从
///       "等第二个事件"改成"看分布尾巴有多长"，用已有的几万个样本换答案。
///
/// 与 `kivo.p12c.decode/5` 的关系：那个只讲视频侧，这个讲"音视频一起跑的时候"。
/// 两者不是替代关系 —— `decode/5` 是纯视频基线，`sync/1` 才回答 G7。
constexpr const char* kSchema = "kivo.p12c.sync/23";

[[nodiscard]] std::string fixed6(double value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    return std::string(buffer);
}

/// 变量名直出用，避免到处写 `<< escape_json_string(...)` 把行拉得很长。
[[nodiscard]] std::string quoted(const std::string& text) {
    return "\"" + mediacodec::escape_json_string(text) + "\"";
}

/// 渲染一段完整分布。
///
/// 这是 C8 "可复算" 的物证：只给 p99，读者只能选择相信；给分布，
/// p99 就退化成一道可以复核的算术题。三个常量（桶宽、下界、桶数）一并导出，
/// 复算方不需要去翻源码里的魔数。
///
/// 非空桶写成 `{"<下标>": 计数}`，**没出现的下标一律为 0** —— 1024 个桶
/// 全写出来是 2 万字符的噪声，而实际只会用到几十个。
void render_distribution(std::ostringstream& out, const SignedHistogram& histogram,
                         const char* indent) {
    const std::string pad(indent);
    out << pad << "\"count\": " << histogram.count() << ",\n";
    out << pad << "\"below\": " << histogram.below_count() << ",\n";
    out << pad << "\"above\": " << histogram.above_count() << ",\n";
    out << pad << "\"min_ns\": " << histogram.min_ns() << ",\n";
    out << pad << "\"max_ns\": " << histogram.max_ns() << ",\n";
    out << pad << "\"bucket_width_ns\": " << SignedHistogram::kBucketWidthNs << ",\n";
    out << pad << "\"low_ns\": " << SignedHistogram::kLowNs << ",\n";
    out << pad << "\"bucket_count\": " << SignedHistogram::kBucketCount << ",\n";
    out << pad << "\"buckets\": {";
    bool first = true;
    const auto& bins = histogram.bins();
    for (std::int32_t index = 0; index < SignedHistogram::kBucketCount; ++index) {
        const std::int64_t count = bins[static_cast<std::size_t>(index)];
        if (count == 0) {
            continue;
        }
        out << (first ? "" : ",") << "\"" << index << "\":" << count;
        first = false;
    }
    out << "}\n";
}

/// 分位数 + 分布的组合对象。
///
/// **它永远是所在对象的最后一个成员**，所以结尾不写逗号。
///
/// 上一版有个 `trailing_comma` 参数，因为 P13-D 把 `per_frame` / `trend` 接在了
/// 它后面。P13-E 把分位数收进 `quantiles.<口径>` 之后，每个块又只剩它自己 ——
/// 参数留着只会让人继续在"这个逗号该不该有"上出错（本轮就真的多写了一个尾逗号，
/// `json.load` 当场报 `Illegal trailing comma`）。删掉参数，让位置只有一个答案：
/// 块自己不带逗号，**外层对象**负责加。
///
/// P13-F 的 `worst` 插在 `max` 与 `histogram` 之间，就是为了不碰这条不变量：
/// `histogram` 仍然永远是最后一个成员。
///
/// P13-N 的 `p50` 精度四格插在 `p50` 与 `p95` 之间，理由与 `worst` 一样：
/// 那四格解释的是**紧挨着它上面那一行那个数**，隔开就成了一堆无主字段。
void render_quantiles(std::ostringstream& out, const SignedHistogram& histogram,
                      const core::pace::DriftExtreme& worst, const char* indent,
                      const core::pace::WorstEpisode* episode = nullptr) {
    const std::string pad(indent);
    const std::int64_t p50 = histogram.percentile_ns(0.50);
    out << pad << "\"samples\": " << histogram.count() << ",\n";
    out << pad << "\"p50\": " << p50 << ",\n";

    // ── `p50` 到底是什么（P13-N）──
    //
    // `percentile_ns(0.50)` 返回的是**桶中心**，桶宽 0.25 ms。真机上 2013/2048
    // 个样本落在 0~250 µs 这一桶里，于是 p50 恒为 125000 —— 而同一份证据里
    // 窗口精确中位数是 74 835。两个数差 66%，JSON 里却没有任何字段能阻止
    // 读者把它读成"两份测量打架"。它们不打架：真值确实在那个桶里，
    // 是桶中心没本事说出来。
    //
    // 所以这里出版三格，让 `p50` 旁边永远站着一句"它可能的范围"：
    //   `p50_range_low/high_ns` —— 真值所在的闭开区间（已与 `[min, max]` 求交，
    //       因为 `percentile_ns` 会把桶中心钳进样本范围，钳过之后可能的范围就窄了）；
    //   `p50_bucket_samples` —— 那个区间里有几个样本。落在一个桶里的样本越多，
    //       `p50` 越接近"这个桶的中心"而不是"这堆数的中位数"。
    // 落在溢出区时 `percentile_ns` 返回精确极值，范围退化成那个点本身
    // （low == high），这是**更准**，不是缺数据。
    //
    // 范围由 `SignedHistogram::percentile_range()` 算，不在这里算 ——
    // 这个文件只在 Android 侧编译，写在这里的钳制逻辑没有桌面单测兜底。
    const core::pace::SignedHistogram::PercentileRange p50_range =
        histogram.percentile_range(0.50);
    out << pad << "\"p50_range_low_ns\": " << p50_range.low_ns << ",\n";
    out << pad << "\"p50_range_high_ns\": " << p50_range.high_ns << ",\n";
    out << pad << "\"p50_bucket_samples\": " << histogram.percentile_bucket(0.50).samples
        << ",\n";
    // 桶宽无条件给：它是**定义**（`SignedHistogram::kBucketWidthNs`），
    // 不是一个观测。有了它，上面那个区间与 p50 的差就能自己复算，
    // 不必相信任何手填数字。
    out << pad << "\"bucket_width_ns\": " << SignedHistogram::kBucketWidthNs << ",\n";

    out << pad << "\"p95\": " << histogram.percentile_ns(0.95) << ",\n";
    out << pad << "\"p99\": " << histogram.percentile_ns(0.99) << ",\n";
    out << pad << "\"max\": " << histogram.max_ns() << ",\n";

    // `worst`：最差那一下**在哪儿**（P13-F）。
    //
    // `max` 只说多大，说不出在片头还是片尾 —— 而这两者对结论的影响完全相反。
    // `valid == false`（这个口径没有样本、或该分布根本没有位置语义）时
    // **整个键不写**，不写零：零会被读成"最差发生在第 0 个样本、媒体时刻 0"，
    // 那是一个看起来完全正常的假观测。
    if (worst.valid) {
        // ── 这是「手慢了」还是「表快了」（P13-P）──
        //
        // 只给 `lateness_ns` 时两者长得一模一样。下面三格让恒等式可以当场复算：
        //
        //     lateness[k] − lateness[k−1] = present_step[k] − due_step[k]
        //
        // `present_step[k]` 就是 `present_jitter` 的样本（`(present[k] − present[k−1]) − T`），
        // 所以 `due_step_ns` 一出版，两侧各占多少就只剩一道减法。
        //
        // 没有 episode（该口径没有邻帧）时**只写原来的四格**，不写零：
        // 零会被读成"目标时刻没动"，那是一个看起来完全正常的假观测。
        const bool has_episode = episode != nullptr && episode->valid;

        out << pad << "\"worst\": {\n";
        out << pad << "  \"index\": " << worst.index << ",\n";
        out << pad << "  \"of\": " << worst.of << ",\n";
        out << pad << "  \"media_ns\": " << worst.media_ns << ",\n";
        // 逗号跟着"后面还有没有成员"走，不跟分支走 —— 上一版在这里多写过一个尾逗号，
        // `json.load` 当场报 Illegal trailing comma（见本文件的另一处注释）。
        out << pad << "  \"lateness_ns\": " << worst.lateness_ns;
        out << (has_episode ? ",\n" : "\n");
        if (has_episode) {
            out << pad << "  \"due_step_ns\": " << episode->due_step_ns << ",\n";
            // P13-Q：从首帧累积到本帧的两侧账。`lateness_at_first_ns` 恒为 0
            // （锚点在第 0 帧建立），出版它是为了让下面那条跨帧恒等式能当场复算。
            out << pad << "  \"cumulative_present_step_ns\": "
                << episode->cumulative_present_step_ns << ",\n";
            out << pad << "  \"cumulative_due_step_ns\": "
                << episode->cumulative_due_step_ns << ",\n";
            out << pad << "  \"lateness_at_first_ns\": "
                << episode->lateness_at_first_ns << ",\n";
            // P13-R：这三格 + `media_ns` 必须满足 `due = mono + (pts − anchor_pts − elapsed)`。
            // 不满足就说明这一帧走的不是正常分支（free_run / pts 不连续），账要另算。
            out << pad << "  \"clock_mono_ns\": " << episode->clock_mono_ns << ",\n";
            out << pad << "  \"clock_elapsed_ns\": " << episode->clock_elapsed_ns << ",\n";
            out << pad << "  \"anchor_pts_ns\": " << episode->anchor_pts_ns << ",\n";
            out << pad << "  \"due_at_ns\": " << episode->due_at_ns << ",\n";
            out << pad << "  \"prev_lateness_ns\": [";
            for (std::int32_t i = 0; i < episode->before_count; ++i) {
                if (i != 0) {
                    out << ", ";
                }
                out << episode->before_ns[i];
            }
            out << "],\n";
            out << pad << "  \"next_lateness_ns\": [";
            for (std::int32_t i = 0; i < episode->after_count; ++i) {
                if (i != 0) {
                    out << ", ";
                }
                out << episode->after_ns[i];
            }
            out << "]\n";
        }
        out << pad << "},\n";
    }

    out << pad << "\"histogram\": {\n";
    render_distribution(out, histogram, (pad + "  ").c_str());
    out << pad << "}\n";
}

}  // namespace

std::string render_av_sync_json(const AvSyncStats& stats) {
    const mediacodec::DecodeStats& video = stats.video;
    const AudioPlaybackStats& audio = stats.audio;
    const core::pace::PaceSummary& pace = stats.pace;

    // 时钟来源只认一件事：**实际驱动呈现的那一个**。
    // 有音频且音频时钟真的发过有效值 → 报音频档；否则老实写 `system`。
    // 用"配置上打算用哪个"来填这一格，等于把降级从证据里抹掉。
    std::string clock_source = "system";
    if (stats.has_audio && audio.clock.queries > 0) {
        clock_source = core::audio::audio_clock_source_name(audio.clock.dominant_source());
    }

    std::string error = video.error;
    if (error.empty()) {
        error = audio.error;
    }
    const bool completed = video.completed && audio.error.empty();

    std::ostringstream out;
    out << "{\n";
    out << "  \"schema\": " << quoted(kSchema) << ",\n";
    out << "  \"generated_at_unix_ms\": " << stats.generated_at_unix_ms << ",\n";

    // P13-AB：探针注入。**三格总是写**（不注入时 `offset_ns = 0`），
    // 而不是"没注入就省略" —— 省略会让"这一轮没注入"和"这个字段还没实现"
    // 长得一样，那是上一张卡吃过的亏（`normal_branch_frames` 满 480 却全是 0）。
    out << "  \"clock_injection\": {\n";
    out << "    \"offset_ns\": " << stats.clock_injection.offset_ns << ",\n";
    out << "    \"arm_after_ns\": " << stats.clock_injection.arm_after_ns << ",\n";
    out << "    \"armed_at_position_ns\": " << stats.clock_injection.armed_at_position_ns
        << ",\n";
    // P13-AD：速率注入。正 = 钟比墙快，与 `clock_ahead`、`playback_rate` 同向。
    // 台阶与速率是**两种不同的输入**：前者考验"当帧还多少"，后者考验
    // "能不能一直追住、不越攒越多"。不出版这一格，两种轮次在证据上无法区分。
    out << "    \"rate_ppm\": " << stats.clock_injection.rate_ppm << "\n";
    out << "  },\n";

    // 门的配置（P13-AD）。校验器据此复算目标残差，不再硬编码 45/5；
    // 关着门的一轮也不会被读成"门判断沉默"。
    out << "  \"drift_gate\": {\n";
    out << "    \"enabled\": " << (pace.drift_gate_enabled ? "true" : "false") << ",\n";
    out << "    \"limit_ns\": " << pace.drift_limit_ns << ",\n";
    out << "    \"band_ns\": " << pace.drift_band_ns << "\n";
    out << "  },\n";

    out << "  \"device\": {\n";
    out << "    \"manufacturer\": " << quoted(stats.device.manufacturer) << ",\n";
    out << "    \"model\": " << quoted(stats.device.model) << ",\n";
    out << "    \"device\": " << quoted(stats.device.device) << ",\n";
    out << "    \"soc_model\": " << quoted(stats.device.soc_model) << ",\n";
    out << "    \"sdk\": " << quoted(stats.device.sdk_int) << ",\n";
    out << "    \"release\": " << quoted(stats.device.release) << ",\n";
    out << "    \"abi\": " << quoted(stats.device.abi) << "\n";
    out << "  },\n";

    out << "  \"codec\": {\n";
    out << "    \"video\": " << quoted(video.codec_name) << ",\n";
    out << "    \"audio\": " << quoted(audio.codec_name) << "\n";
    out << "  },\n";

    out << "  \"source\": {\n";
    out << "    \"label\": " << quoted(stats.source_label) << ",\n";
    out << "    \"bytes\": " << stats.source_bytes << ",\n";
    out << "    \"has_video\": " << (stats.has_video ? "true" : "false") << ",\n";
    out << "    \"has_audio\": " << (stats.has_audio ? "true" : "false") << ",\n";
    out << "    \"duration_us\": " << video.media_duration_us << ",\n";
    out << "    \"video\": {\"mime\": " << quoted(video.mime) << ", \"width\": " << video.width
        << ", \"height\": " << video.height << "},\n";
    out << "    \"audio\": {\"mime\": " << quoted(audio.mime)
        << ", \"sample_rate\": " << audio.declared_sample_rate
        << ", \"channel_count\": " << audio.declared_channel_count << "}\n";
    out << "  },\n";

    // ── 卡片点名的五个标量 ──
    out << "  \"frame_count\": " << video.frames_rendered << ",\n";
    out << "  \"frames_dropped\": " << video.frames_dropped_late << ",\n";
    out << "  \"xrun_count\": " << audio.output.xrun_count << ",\n";
    out << "  \"clock_source\": " << quoted(clock_source) << ",\n";
    out << "  \"timestamp_fallback_count\": "
        << (audio.clock.published_frames_read + audio.clock.published_frames_written +
            audio.clock.published_frozen)
        << ",\n";

    // ── 音画偏差 ──
    //
    // 能不能算，取决于三件事，缺一不可：
    //   ① 片源真有音轨（`system` 时钟下这个量只是"相对理想节拍的偏移"）；
    //   ② 时钟真是音频档；
    //   ③ 呈现路径是 `cpu_paced` —— 帧在 `due_at` **当刻**交出。
    //
    // 第 ③ 条是一次真机测量逼出来的：`kSurfaceAtTime` 不睡，连交 3 帧，
    // 于是 `lateness` 恒等于提交提前量（实测 p50 = −99.875 ms = 3 帧，
    // 174/180 个样本甚至越过了直方图 −128 ms 的下界），跟音画同步没关系。
    //
    // 也就是说：**"提前 3 帧交出去"和"音画不同步"在数值上长得一样，必须靠路径区分。**
    // 上屏时刻的同步误差在 NDK 里拿不到（`21-p12c6b-present-backpressure.md` §二），
    // 要它得等 P12-D 的 present fence。本节宁可写 `measurable: false`，也不编一个数。
    const bool audio_clock = clock_source != "system";
    const bool paced_at_present = video.present_schedule == "cpu_paced";
    const bool drift_measurable =
        stats.has_audio && audio_clock && paced_at_present && pace.lateness.count() > 0;

    out << "  \"av_drift\": {\n";

    // ── 口径先于数字，且结构上不可绕过 ──
    //
    // P13-C 吃过一次教训：`audio.opened` 恒为 false，单看那个字段完全合法，
    // 只有和 `clock_source` 并排才暴露矛盾。P13-D 撞上的是同一类错的第二个实例：
    // `p99 = 1.6 ms` 单独看无所谓对错，**配不上"逐帧"还是"抽稀"就不可比** ——
    // 而它当时躺在 `av_drift.p99` 这种不带口径的键下面，写表格的人复制一个数，
    // 永远不会想起去看三行之外的 `sampling.mode`。
    //
    // P13-E 的修法不是"记得看 mode"，而是**让裸的 p99 在结构上不存在**：
    // 分位数一律挂在 `quantiles.<口径>` 下，想取一个数就必须先说出是哪个口径。
    // 抽稀不成立时 `quantiles.media_time_decimated` **整块缺席**，不拿逐帧的数顶上 ——
    // 缺席会让表格空一格，冒充会让人算出一个看起来很正常的错数。
    // 抽稀块"有没有"和"能不能用"是两件事（P13-G）。
    //
    // 原先这里只判 `sampled_samples > 0`。那条判据的问题是：**1 个样本也算有**。
    // 而 1 个样本的 p50/p95/p99/max 四个数完全相同，长得像一份正常数据，
    // 于是 E1/E2/E3/F1/F2′ 全套判据都会放它过去（见 `68-p13g-design.md` §二）。
    // 缺 pts 的流正好会产出一份这样的东西：首帧接受、其后每个 pts 都是 0、
    // 全部 `skipped`。
    //
    // 现在要求"至少两点且跨度为正"才算抽稀成立。这个门槛只管**结构**
    // （是不是抽稀数据），不管**可信度**（斜率能不能看）—— 后者归
    // `DriftTrend::kMinSamples` / `kMinSpanNs`，两件事不能用一个阈值。
    const bool decimated =
        pace.sampled_samples > 0 &&
        core::pace::decimated_block_is_usable(pace.sampled_samples, pace.drift_media_span_ns);

    // 口径名与"为什么是这个口径"必须成对。三种情况分开说，不许用一句通用的
    // "没传 pts" 去解释所有空序列 —— 不可测的那些轮次里，真实原因往往是
    // 没音轨或走了 surface_at_time，跟 pts 一点关系都没有。
    // 一个解释错的 reason 比没有 reason 更坏：它会把排查引到反方向。
    core::pace::SamplingMode mode = core::pace::SamplingMode::kMediaTimeDecimated;
    std::string sampling_reason;
    if (!drift_measurable) {
        mode = core::pace::SamplingMode::kNotMeasured;
        sampling_reason =
            "av_drift is not measurable on this run; the reason is in unavailable_reason below "
            "(no audio attached / clock_source is system / present_schedule is surface_at_time)";
    } else if (!decimated) {
        mode = core::pace::SamplingMode::kPerFrame;
        // 理由**必须**由 `decimated_unusable_reason()` 给，不许在这里另写一句。
        //
        // 上一版这里写的是 "no media pts was supplied to note_presented()"。
        // 那句话描述了一件不可能发生的事：`note_presented()` 的签名就要求 pts，
        // 且它的首帧分支无条件接受 —— 只要它被调过一次，`sampled_samples` 就 ≥ 1。
        // 所以 `kPerFrame` 当时是**死路径**，而那句 reason 是编的
        // （完整的链见 `68-p13g-design.md` §一）。
        //
        // 现在门槛改在"样本够不够证明媒体时间在推进"，这条路径真的会被走到，
        // 理由也有一个唯一出处。改文案要改的是那个函数，不是这里。
        sampling_reason = core::pace::decimated_unusable_reason(
            pace.sampled_samples, pace.drift_media_span_ns);
    }

    const std::int64_t sample_interval_ns = core::pace::DriftSeries::kSampleIntervalNs;
    // 分子分母必须同口径（P13-H）。
    //
    // `sampled_samples` 是**累计**接受数（环形覆盖后继续涨），
    // 所以分母只能用 `drift_media_span_ns`（累计跨度）。
    // 上一版这里用的是 `pace.drift.span_ns` —— 那是**窗口**跨度，封顶 102.35 s。
    // 两者的商在 8 s / 35 s 片源上恰好对，超过 102.4 s 之后随片长线性增长：
    // 130 s 报 25.4 Hz、600 s 报 117 Hz，而旁边写着 `target_hz: 20`。
    // 那个数没有上界，也没有判据拦它 —— 校验器复算的是同一个除法
    // （`70-p13h-design.md` §一）。
    //
    // 现在两个操作数在 JSON 里**相邻**（`accepted` 后面紧跟 `media_span_ns`），
    // 谁想再算一遍这个除法，都不必去别处找分母。
    const double observed_hz =
        (pace.drift_media_span_ns > 0)
            ? static_cast<double>(pace.sampled_samples) * 1e9 /
                  static_cast<double>(pace.drift_media_span_ns)
            : 0.0;

    if (drift_measurable) {
        out << "    \"measurable\": true,\n";
        out << "    \"measured_at\": \"present\",\n";
        out << "    \"definition\": "
            << quoted("(audio clock elapsed) - (video pts elapsed), sampled at present_at; "
                      "positive means video is behind audio")
            << ",\n";
    } else {
        out << "    \"measurable\": false,\n";
    }

    out << "    \"sampling\": {\n";
    // 名字取自 `core/pace/sampling_mode` 的唯一一张表 —— 报告层不许自己再写一遍字面量，
    // 否则 C++ 一处、Python 校验器一处，两边迟早各说各话。
    out << "      \"mode\": " << quoted(core::pace::sampling_mode_name(mode)) << ",\n";
    // 这一格是本卡的全部意义：它**指向**唯一能放进"30fps vs 60fps"那张表的分位数集合。
    // 写成一个路径而不是布尔值：布尔只说明"有得比"，路径说明"比的是哪一份"。
    // 抽稀不成立时写 null，而不是写 "per_frame" —— 逐帧根本不能跨帧率比。
    if (drift_measurable && decimated) {
        out << "      \"cross_frame_rate_set\": "
            << quoted("quantiles.media_time_decimated") << ",\n";
    } else {
        out << "      \"cross_frame_rate_set\": null,\n";
    }
    out << "      \"interval_ns\": " << sample_interval_ns << ",\n";
    out << "      \"target_hz\": " << (1'000'000'000LL / sample_interval_ns) << ",\n";
    out << "      \"frames_observed\": " << pace.drift_observed_frames << ",\n";
    out << "      \"accepted\": " << pace.sampled_samples << ",\n";
    // 紧跟 `accepted`：这是它的分母，不是别处的跨度。
    // `trend.span_ns` 是窗口跨度，答的是另一个问题（回归用了多长的时间），
    // 两者在未覆盖时相等、覆盖后必然不等 —— 摆在一起容易被当成同一个数。
    out << "      \"media_span_ns\": " << pace.drift_media_span_ns << ",\n";
    out << "      \"skipped\": " << pace.drift_skipped_frames << ",\n";
    // 播放空洞：一格没采到时 20 Hz 的实际节奏就掉了。只报不补 —— 补等于编数据。
    out << "      \"unobserved_cells\": " << pace.drift_unobserved_cells << ",\n";
    out << "      \"observed_hz\": " << fixed6(observed_hz) << ",\n";
    out << "      \"reason\": ";
    if (mode == core::pace::SamplingMode::kPerFrame) {
        // 两半拼起来：前半是"为什么抽稀不成立"（唯一出处是那个函数），
        // 后半是"所以你能拿到什么"（口径警告，与具体原因无关）。
        // 不把后半塞进那个函数：它会随原因重复三遍，改一次漏两处。
        out << quoted(std::string(sampling_reason) +
                      "; only per-frame quantiles exist on this run and they are NOT "
                      "comparable across frame rates");
    } else {
        out << quoted(sampling_reason);
    }
    out << "\n";
    out << "    },\n";

    // ── 分位数：按口径分块，没有"默认那一份" ──
    //
    // 不可测时给一个**空对象**而不是给 0：`{}` 说的是"一份都没有"，
    // `0` 说的是"量到了零"—— 后者是个能算进平均值的假观测。
    out << "    \"quantiles\": {";
    if (!drift_measurable) {
        out << "},\n";
    } else {
        out << "\n";
        if (decimated) {
            out << "      \"media_time_decimated\": {\n";
            render_quantiles(out, pace.sampled_lateness, pace.sampled_worst, "        ");
            out << "      },\n";
        }
        // 逐帧那份一直在：G6 的 `over_budget_frames` 必须逐帧数，
        // 抽稀会漏掉恰好落空的那几帧，所以不能只剩抽稀那一份。
        out << "      \"per_frame\": {\n";
        render_quantiles(out, pace.lateness, pace.frame_worst, "        ",
                         &pace.frame_episode);
        out << "      }\n";
        out << "    },\n";
    }

    if (drift_measurable) {
        // ── 漂移 ──
        //
        // 分位数对"慢速线性增长"是瞎的：音频时钟快 0.1% 时每秒多偏 1 ms，
        // 8 秒只累积 8 ms，而抖动本身就有 ±1 ms —— p99 看不出任何异常，
        // 放到 10 分钟的长片里却是 600 ms 的音画不同步。所以斜率要单独一把尺。
        const core::pace::DriftTrend& trend = pace.drift;
        out << "    \"trend\": {\n";
        out << "      \"usable\": " << (trend.usable ? "true" : "false") << ",\n";
        // `usable == false` 时非空。宁可写"量不出来"，也不许报一个 0 让人读成"没漂移"。
        out << "      \"unusable_reason\": " << quoted(trend.unusable_reason) << ",\n";
        out << "      \"windows\": " << trend.window_count << ",\n";
        out << "      \"span_ns\": " << trend.span_ns << ",\n";
        // ── 三个数说清"窗口里为什么只有这些点"（P13-I）──
        //
        // 上一版只有 `segment_samples` 一个数加一个 `truncated` 布尔。
        // 那份数据回答不了下面这个问题：窗口 1800 点、累计 3400 点，差在哪？
        // 是覆盖（前面被环形吃掉了）还是 seek（前面那段换了时间轴）？——
        // 两种原因对应两种完全不同的读法，而上一版的 `truncated` 会把后者也说成前者。
        //
        // 现在三个数各有各的问题，且互相能验：
        //   `capacity`           环形能装几个（导出常量，读者才能自己算 min）
        //   `segment_accepted`   本段时间轴采了几个（seek 之后从 0 重新起算）
        //   `window_sample_count` 窗口里还剩几个 = min(上两者)
        // 覆盖 ⇔ `segment_accepted > capacity`；跨段 ⇔ `segments > 1`。
        // 两个原因各有各的字段，读者不必去猜。
        out << "      \"capacity\": " << core::pace::DriftSeries::kCapacity << ",\n";
        out << "      \"segment_accepted\": " << pace.drift_segment_samples << ",\n";
        out << "      \"window_sample_count\": " << pace.window_lateness.samples << ",\n";
        // 环形覆盖 / seek 会改变"这份趋势描述的是哪一段"，必须一并说清。
        out << "      \"truncated\": " << (trend.truncated ? "true" : "false") << ",\n";
        out << "      \"segments\": " << trend.segments << ",\n";
        // ── 段内的那个洞（P13-M）──
        //
        // `span_ns` 是"末 − 首"，空洞**落在里面**。一个 10 s 的洞会让跨度多算
        // 10 s，`slope_ppm × span` 的累积量跟着偏 —— 而 `segments` 一个字都不会说，
        // 因为那按定义是同一段（同一锚点，中间卡住了）。
        //
        // 这两个字段不参与任何判定，只回答"这条直线横跨了多宽的一个洞"。
        // 放在 `segments` 后面而不是末尾：它们是读 `span_ns` 的前提，
        // 隔几十行再出现就等于让读者先乘错一次。
        out << "      \"segment_largest_gap_cells\": " << trend.segment_largest_gap_cells << ",\n";
        out << "      \"segment_contiguous\": " << (trend.segment_contiguous ? "true" : "false")
            << ",\n";
        out << "      \"first_window_median_ns\": " << trend.first_window_median_ns << ",\n";
        out << "      \"last_window_median_ns\": " << trend.last_window_median_ns << ",\n";
        out << "      \"delta_median_ns\": " << trend.delta_median_ns << ",\n";
        // ── 整批点的精确参照（P13-N）──
        //
        // 紧挨着 `delta_median_ns` 而不是丢到文档里：读者拿 `quantiles.*.p50`
        // （桶中心 125000）和这三个中位数（真实 74 835）并排看的时候，
        // 中间必须有一个**同批点的精确数**可以对照，否则那 66% 的差只能靠解释。
        //
        // 只在 `window_median_exact == true` 时出版：有窗退回桶中心时，
        // 这个"中位数的中位数"混着桶中心与真值，拿去对照只会把两个误差叠加。
        // 那时两格都不写 —— 缺席本身就是一句"这一轮没有精确参照"。
        if (trend.window_median_exact) {
            out << "      \"median_of_window_medians_ns\": "
                << trend.median_of_window_medians_ns << ",\n";
        }
        out << "      \"slope_ppm\": " << fixed6(trend.slope_ppm) << ",\n";
        // 紧跟在斜率后面是**防腐**，不是排版：
        // 这两个数分开摆，读者就会单独引用 `slope_ppm` 那六位小数 ——
        // 而它的可信度全看下面这个 SE。`r2 = 5.5e-05` 时 SE 比斜率大一个数量级，
        // 单独引用它等于把噪声包装成结论（P13-J §一）。
        //
        // P13-K 起这个数是 **HAC 修正后**的。名字没变、语义变了 ——
        // 保留旧名是因为它才是"该用的那个数"，不该让读者在 `_iid` / `_hac`
        // 之间二选一；旧口径原地改名 `_iid`，不会被谁顺手引用。
        out << "      \"slope_ppm_std_error\": " << fixed6(trend.slope_ppm_std_error) << ",\n";
        // 旧口径与膨胀比并排放在新值下面：没有这两个数，"我们换成 HAC 了"
        // 就是一句无法检验的声明 —— 读者无从知道修正到底动了多少。
        out << "      \"slope_ppm_std_error_iid\": " << fixed6(trend.slope_ppm_std_error_iid)
            << ",\n";
        // SE 之比（不是方差之比）：眼前两个 SE 直接相除就能复算。
        out << "      \"slope_ppm_std_error_inflation\": "
            << fixed6(trend.slope_ppm_std_error_inflation) << ",\n";
        // 带宽与残差一阶自相关：前者让人能拿原始序列复算 HAC，
        // 后者一句话解释"为什么要修正"。少一个，这套数就只能被相信。
        out << "      \"hac_lags\": " << trend.hac_lags << ",\n";
        out << "      \"hac_lag1_autocorrelation\": "
            << fixed6(trend.hac_lag1_autocorrelation) << ",\n";
        // `false` ⇒ 样本太少没估出来，上面的 SE 其实是 IID 旧值。
        // 不写这一位，"修正过"与"没修正"在 JSON 上长得一模一样。
        out << "      \"hac_estimated\": " << (trend.hac_estimated ? "true" : "false") << ",\n";
        out << "      \"slope_ci_low_ppm\": " << fixed6(trend.slope_ci_low_ppm) << ",\n";
        out << "      \"slope_ci_high_ppm\": " << fixed6(trend.slope_ci_high_ppm) << ",\n";
        // `false` 的正确读法：漂移小于 CI 半宽，**不是**"漂移为零"。
        // 这正是 `slope_ppm` 一直缺的那句翻译。
        out << "      \"slope_detectable\": " << (trend.slope_detectable ? "true" : "false")
            << ",\n";
        // ── 看得见 ≠ 要紧（P13-L）──
        //
        // 上一行刚问完"这个斜率与 0 分得开吗"，这四个字段就在这里回答"那要不要紧"。
        // 必须紧挨着，不能挪到 trend 末尾：挪开就等于让读者自己去乘那个 ppm，
        // 而 P13-K 的 r3 正是这么被读成"要出事"的 —— `detectable = true`，
        // 累积量 17 µs，比一次 vsync 抖动还小。
        //
        // 三个数是**事实**，一个词是**结论**：
        //   `drift_accumulated_ns`        点估计（带符号，正 = 越来越晚）
        //   `drift_accumulated_upper_ns`  置信带远端积出的保守上界（非负）
        //   `drift_budget_ns`             判定用的额度（45 ms，ITU-R BT.1359）
        //   `drift_verdict`               unknown / within_budget / over_budget
        //
        // 三态不是布尔：`unknown` 表"这份趋势没资格下结论"，把它和
        // `within_budget` 一起印成 `false`，就是让一次失败伪装成一次通过。
        //
        // 累积量只覆盖 `span_ns` 这一段，不是整场 —— 斜率不能拿去做外推，
        // 一份 600 秒的片源若趋势只覆盖最后 102 秒，这里说的也只是那 102 秒。
        out << "      \"drift_accumulated_ns\": " << trend.drift_accumulated_ns << ",\n";
        out << "      \"drift_accumulated_upper_ns\": " << trend.drift_accumulated_upper_ns
            << ",\n";
        out << "      \"drift_budget_ns\": " << trend.drift_budget_ns << ",\n";
        out << "      \"drift_verdict\": "
            << quoted(core::pace::drift_verdict_name(trend.drift_verdict)) << ",\n";
        out << "      \"slope_from_windows_ppm\": " << fixed6(trend.slope_from_windows_ppm)
            << ",\n";
        // 说一个 0 而不说尺子上有几个刻度，是这份证据从 P13-D 到 P13-I 一直犯的错。
        // `slope_from_windows_ppm == 0` 只有在 `window_median_exact == true` 时才
        // 能读成"没漂"，否则它只是"八个数都被量化到同一个格点"。
        out << "      \"window_median_exact\": "
            << (trend.window_median_exact ? "true" : "false") << ",\n";
        out << "      \"window_median_fallback_windows\": "
            << trend.window_median_fallback_windows << ",\n";
        // 光有斜率没有 r2，噪声会被读成漂移 —— 这个字段是斜率的准入条件，不是装饰。
        out << "      \"r2\": " << fixed6(trend.r2) << ",\n";
        // ── 半程斜率：一条线的两种来历（P13-K 诊断）──
        //
        // 全序列斜率分不清"持续漂移"和"中途跳了一下"：两种情形斜率一样，
        // 结论却相反（前者会一直累积，后者不会）。两个半程各拟合一次，
        // 同号才说明整段朝一个方向走。
        // `slope_halves_agree` 只是诊断，不带阈值也不参与任何判定 ——
        // 斜率接近 0 时符号由噪声决定，那时 true/false 就是掷硬币。
        out << "      \"slope_first_half_ppm\": " << fixed6(trend.slope_first_half_ppm) << ",\n";
        out << "      \"slope_second_half_ppm\": " << fixed6(trend.slope_second_half_ppm) << ",\n";
        out << "      \"slope_halves_agree\": " << (trend.slope_halves_agree ? "true" : "false")
            << ",\n";
        // ── 半程一致性：把"差多少"和"差多少算差"一起出版（P13-O）──
        //
        // 上面那个布尔只说同不同号。真机 13 份证据里 8 份报 true，其中 3 份
        // 两半程量级差 5 倍以上；最极端的一份两半程都是正的、全序列斜率却是负的。
        // 这一组才是该读的那一格：`gap` 与 `pooled` 必须成对出现，
        // 单独一个 gap 会让读者拿它去和一把不存在的尺子比。
        //
        // `estimated == false` 时误差那一组全部写 0 并由 `relation = unknown` 兜住；
        // 但 `gap_ppm` 与 `cumulative_gap_ns` 照写 —— 那两个只依赖斜率，
        // 而斜率总是拟合得出来的。
        const pace::HalvesAgreement& halves = trend.halves;
        out << "      \"slope_first_half_std_error_ppm\": "
            << fixed6(halves.first_std_error_ppm) << ",\n";
        out << "      \"slope_second_half_std_error_ppm\": "
            << fixed6(halves.second_std_error_ppm) << ",\n";
        out << "      \"slope_first_half_ci_low_ppm\": " << fixed6(halves.first_ci_low_ppm)
            << ",\n";
        out << "      \"slope_first_half_ci_high_ppm\": " << fixed6(halves.first_ci_high_ppm)
            << ",\n";
        out << "      \"slope_second_half_ci_low_ppm\": " << fixed6(halves.second_ci_low_ppm)
            << ",\n";
        out << "      \"slope_second_half_ci_high_ppm\": " << fixed6(halves.second_ci_high_ppm)
            << ",\n";
        out << "      \"slope_halves_gap_ppm\": " << fixed6(halves.gap_ppm) << ",\n";
        out << "      \"slope_halves_pooled_error_ppm\": " << fixed6(halves.pooled_error_ppm)
            << ",\n";
        // `z` 可能是 +inf（两侧残差都恒为 0 且斜率不同）。`%.6f` 会打印 `inf`，
        // 那不是合法 JSON —— 整个文件会因此在任何解析器里炸掉。
        // 写 null 并让校验器按"pooled == 0"那条规则解释（见 O4）。
        out << "      \"slope_halves_z\": ";
        if (std::isfinite(halves.z)) {
            out << fixed6(halves.z);
        } else {
            out << "null";
        }
        out << ",\n";
        out << "      \"slope_halves_cumulative_gap_ns\": " << halves.cumulative_gap_ns << ",\n";
        // 两段真实跨度：O6 复算 `cumulative_gap_ns` 的第二个输入。
        // 没有它们，那个整数就只能被"验成非负"，而它的实际含义
        // `|first×span1 − second×span2|` 里有两个未知量。
        out << "      \"slope_first_half_span_ns\": " << halves.first_span_ns << ",\n";
        out << "      \"slope_second_half_span_ns\": " << halves.second_span_ns << ",\n";
        out << "      \"slope_halves_resolution_ppm\": " << fixed6(halves.resolution_ppm) << ",\n";
        out << "      \"slope_halves_budget_equivalent_ppm\": "
            << fixed6(halves.budget_equivalent_ppm) << ",\n";
        out << "      \"slope_halves_resolution_sufficient\": "
            << (halves.resolution_sufficient ? "true" : "false") << ",\n";
        out << "      \"slope_halves_relation\": "
            << quoted(pace::halves_relation_name(halves.relation)) << ",\n";
        // 8 个点是"可复算"的物证：第三方能拿下面三行自己拟合一遍，看是不是同一个量级。
        out << "      \"window_start_ns\": [";
        for (std::int32_t i = 0; i < trend.window_count; ++i) {
            out << (i == 0 ? "" : ", ")
                << trend.window_start_ns[static_cast<std::size_t>(i)];
        }
        out << "],\n";
        out << "      \"window_median_ns\": [";
        for (std::int32_t i = 0; i < trend.window_count; ++i) {
            out << (i == 0 ? "" : ", ")
                << trend.window_median_ns[static_cast<std::size_t>(i)];
        }
        out << "],\n";
        out << "      \"window_samples\": [";
        for (std::int32_t i = 0; i < trend.window_count; ++i) {
            out << (i == 0 ? "" : ", ")
                << trend.window_samples[static_cast<std::size_t>(i)];
        }
        out << "],\n";
        // ── 趋势自带的那批点，自带它们的分布（P13-I）──
        //
        // 与 `quantiles.media_time_decimated` 是**两批不同的样本**：
        // 那个是整场累计（环形覆盖后继续加、seek 之后也继续加），
        // 这个是 `trend` 刚刚回归过的那 N 个点。把两个数并排放而不说明，
        // 读表的人必然当成同一批 —— 一份 130 秒的证据里，
        // p99 覆盖 2600 点、slope 覆盖 2048 点，谁也不会想起去看 `truncated`。
        //
        // 判据 I1 用 `Σ window_samples == window_sample_count` 把它钉住：
        // 那两个数一个来自 8 趟分窗循环、一个来自 `DriftSeries::size()`，
        // 算法不同、路径不同，只有它们真的数的是同一批点才会相等。
        //
        // **不写 `worst`**（传 `kNoExtreme`）：累计口径已经有一个 `worst`，
        // 再给窗口一个同名不同口径的，就是 P13-E 花一整张卡消灭的那种东西。
        // 想知道"最差那一下在哪儿"，答案在累计那一份里，且带着 `media_ns` 能自己对齐。
        out << "      \"window_quantiles\": {\n";
        render_quantiles(out, pace.window_lateness.lateness, kNoExtreme, "        ");
        out << "      }\n";
        out << "    }\n";
    } else {
        // 这里**不能**再写一次 `"measurable": false` —— 它在上面 `sampling` 之前
        // 已经按同一个条件输出过了。重复键在 JSON 里是"后者覆盖前者"或直接报错，
        // 总收口（verify_p12_all 层1）正是靠"重复键"这一条抓到的。
        out << "    \"unavailable_reason\": ";
        if (!stats.has_audio) {
            // `has_audio` 是"这次会话有没有接音频"，不是"片源有没有音轨"。
            // `kivo.audio off` 会让它变 false，而片源其实带音轨 —— 曾经这里直接写
            // "source has no audio track"，在关音频的对照轮里就是一句假话。
            // 不猜是哪一种，把两种可能都列出来。
            out << quoted("no audio is attached to this session: either the source has no audio "
                          "track, or audio was disabled for this run (kivo.audio off). "
                          "Without an audio timeline there is nothing to compare video pts "
                          "against");
        } else if (!audio_clock) {
            out << quoted("clock_source is system; the per-frame offset would only measure "
                          "schedule offset, not A/V sync");
        } else if (!paced_at_present) {
            out << quoted("present_schedule is surface_at_time: frames are submitted ~3 frames "
                          "early, so the per-frame offset equals the submit lead (measured "
                          "p50 = -99.875 ms) and is not A/V drift; measuring on-screen sync "
                          "requires present fences (see 21-p12c6b-present-backpressure.md 2)");
        } else {
            out << quoted("no lateness samples were recorded at present time");
        }
        out << "\n";
    }
    out << "  },\n";

    out << "  \"present_jitter\": {\n";
    // 两条路径的采样点不同，必须标明 —— 否则同一组数字会被当成同一件事：
    // `cpu_paced` 记在"交出前的一瞬"，`surface_at_time` 记在"提交给合成器的一瞬"。
    out << "    \"measured_at\": "
        << quoted(video.present_schedule == "cpu_paced" ? "present" : "submit") << ",\n";
    // 这个块**没有位置**：`note_interval()` 收到的只是两个时刻相减，
    // 调用方不给媒体时刻。传一个 invalid 的极值 = 不写 `worst` 键，
    // 而不是写一个零 —— 零会被读成"最差发生在第 0 个样本"。
    render_quantiles(out, pace.interval, kNoExtreme, "    ");
    out << "  },\n";

    // ── 播放速率：唯一一个不与节拍器结论循环的检查 ──
    //
    // 分子是媒体时间（首帧→末帧的 pts），分母是墙钟时间（同一对帧的交出时刻）。
    // `av_drift` / `schedule_lead` 都是拿节拍器自己的目标时刻当基准，所以节拍器
    // 整体跑偏时它们可能依然"完美"；这个比值只看墙钟，跑偏就露出来。
    //
    // 刻意**不用**"音频时钟跨度 ÷ 视频 pts 跨度"来算：音频会话总比视频多活一会儿
    // （排空、轮询间隔、收尾），那个差值里混的是收尾耗时，不是分叉。
    // 第一版就是这么写出 35–104 ms 的假"分叉"的，而同轮 `av_drift.p99` 只有 0.125 ms ——
    // 两个数自相矛盾，说明错在窗口。音频停止时刻的原始位置仍如实记录，供人自行核对。
    // 媒体跨度按**交出去的帧数**算，不是按"上了屏的帧数"。
    // 换面空窗期那几十帧没上屏，但 pts 照样走过去了（解码器把它们的缓冲还回来了）。
    // 只数 `frames_rendered` 会把跨度算短，速率就被误判成"放慢了" ——
    // D1 第一版就是这么把 0.92 报出来的，而那 92% 只是空窗期的占比。
    const std::int64_t handed_off = video.frames_rendered + video.frames_dropped_late +
                                    video.frames_discarded_no_surface;
    const std::int64_t video_span_ns =
        (handed_off > 1) ? (handed_off - 1) * pace.frame_duration_ns : 0;
    const std::int64_t wall_span_ns = pace.last_present_mono_ns - pace.first_present_mono_ns;

    // `surface_at_time` 路径的交出时刻比 `due_at` 早一个固定提前量，而且这个提前量
    // 是**从 0 涨到 3 帧**的（首帧建锚时提前量为 0，之后被 cap 卡住）。
    // 于是"首末交出时刻之差"比媒体跨度短了整整一个提前量 —— 直接算速率会读出 1.017，
    // 看起来像"放快了 1.7%"，其实是提前量填进去的。
    //
    // 修正用 `schedule_lead` 的 p50（`cpu_paced` 路径没有样本 → 0 → 不修正）。
    // 这是**减法**不是抹平：修正项单独报出来，读者可以拒绝它自己重算。
    const std::int64_t lead_correction_ns =
        (pace.scheduled_samples > 0 && pace.schedule_lead.percentile_ns(0.50) < 0)
            ? -pace.schedule_lead.percentile_ns(0.50)
            : 0;
    const std::int64_t corrected_span_ns = wall_span_ns + lead_correction_ns;
    const double playback_rate = (corrected_span_ns > 0 && video_span_ns > 0)
                                     ? static_cast<double>(video_span_ns) /
                                           static_cast<double>(corrected_span_ns)
                                     : 0.0;

    out << "  \"timeline\": {\n";
    out << "    \"video_pts_span_ns\": " << video_span_ns << ",\n";
    out << "    \"present_wall_span_ns\": " << wall_span_ns << ",\n";
    out << "    \"lead_correction_ns\": " << lead_correction_ns << ",\n";
    out << "    \"corrected_wall_span_ns\": " << corrected_span_ns << ",\n";
    out << "    \"playback_rate\": " << fixed6(playback_rate) << ",\n";
    // 原始事实照留：有人要按自己的窗口重算时用得上。
    out << "    \"anchor_audio_position_ns\": " << pace.anchor_position_ns << ",\n";
    out << "    \"audio_position_at_video_stop_ns\": "
        << stats.audio_position_at_video_stop_ns << ",\n";
    out << "    \"note\": "
        << quoted("playback_rate = media time / wall time between first and last hand-off, "
                  "with the submit-lead ramp removed. The audio position at video stop "
                  "includes teardown latency (poll interval + drain) and must not be compared "
                  "against the pts span directly")
        << "\n";
    out << "  },\n";

    // ── 锚点：`av_drift` 是相对量，被吸收的常量必须能单独看到 ──
    out << "  \"anchor\": {\n";
    out << "    \"video_pts_ns\": " << pace.anchor_pts_ns << ",\n";
    out << "    \"audio_position_ns\": " << pace.anchor_position_ns << ",\n";
    out << "    \"note\": "
        << quoted("av_drift is measured relative to this anchor; the constant absorbed here "
                  "is audio_position_ns at the first video frame")
        << "\n";
    out << "  },\n";

    // ── 视频侧 ──
    out << "  \"video\": {\n";
    out << "    \"frames_received\": " << video.frames_received << ",\n";
    out << "    \"frames_released\": " << video.frames_released << ",\n";
    out << "    \"frames_rendered\": " << video.frames_rendered << ",\n";
    out << "    \"frames_dropped_late\": " << video.frames_dropped_late << ",\n";
    // D1：换面空窗期丢的帧。与"节拍判它太晚"分开记，否则一次切后台看起来像节拍崩了。
    out << "    \"frames_discarded_no_surface\": " << video.frames_discarded_no_surface << ",\n";
    out << "    \"samples_submitted\": " << video.samples_submitted << ",\n";
    out << "    \"samples_rejected\": " << video.samples_rejected << ",\n";
    out << "    \"wall_us\": " << video.wall_micros << ",\n";
    out << "    \"first_frame_latency_us\": " << video.first_frame_latency_us << ",\n";
    out << "    \"decode_fps\": " << fixed6(video.decode_fps) << ",\n";
    out << "    \"speed_vs_realtime\": " << fixed6(video.speed_vs_realtime) << ",\n";
    out << "    \"pipeline_mode\": " << quoted(video.pipeline_mode) << ",\n";
    out << "    \"present_frames\": " << (video.present_frames ? "true" : "false") << ",\n";
    out << "    \"present_schedule\": " << quoted(video.present_schedule) << ",\n";
    // D1：换面。`rebuilds == 0` 时其余字段保持 -1 —— 不写 0，
    // 0 会被读成"恢复耗时 0ms"。
    out << "    \"surface_rebuild\": {\"rebuilds\": " << video.surface_rebuild.rebinds
        << ", \"gap_ms\": " << video.surface_rebuild.last_gap_ms
        << ", \"resume_ms\": " << video.surface_rebuild.last_resume_ms
        << ", \"frames_lead\": " << video.surface_rebuild.last_frames_lead << "}\n";
    out << "  },\n";

    // ── 节拍侧面 ──
    out << "  \"pace\": {\n";
    // 没有 `enabled` 这个开关可查：节拍是"配了就有"。用 frames_decided 说话，
    // 0 就是没开节拍 —— 不另外编一个可能与事实不符的布尔量。
    out << "    \"enabled\": " << (pace.frames_decided > 0 ? "true" : "false") << ",\n";
    out << "    \"frames_decided\": " << pace.frames_decided << ",\n";
    out << "    \"presented\": " << pace.presented << ",\n";
    out << "    \"dropped\": " << pace.dropped << ",\n";
    out << "    \"over_budget_frames\": " << pace.over_budget_frames << ",\n";
    // P13-P：目标时刻动过几帧，以及"这一路参数到底接没接上"。
    // 后者不是调试字段：`due_step_frames` 为 0 有两种读法（没位移 / 没接线），
    // 只出版前者，读者会永远读成第一种。`due_step_supplied < presented` 时，0 作废。
    out << "    \"due_step_frames\": " << pace.due_step_frames << ",\n";
    out << "    \"due_step_supplied\": " << pace.due_step_supplied << ",\n";
    // P13-Q：跨帧账。三个数在 `dropped == 0` 时严格闭合
    // （`last − first == sum_present − sum_due`），残差单独出版 —— 它非 0 时
    // 说明中间有帧没被记账，别拿这三个数去解释整段。
    out << "    \"first_lateness_ns\": " << pace.first_lateness_ns << ",\n";
    out << "    \"last_lateness_ns\": " << pace.last_lateness_ns << ",\n";
    out << "    \"sum_present_step_ns\": " << pace.sum_present_step_ns << ",\n";
    out << "    \"sum_due_step_ns\": " << pace.sum_due_step_ns << ",\n";
    out << "    \"closure_gap_ns\": " << pace.closure_gap_ns << ",\n";
    // P13-R：四个跨度。正常分支上 `due_span == arrive_mono_span + pts_span − elapsed_span`
    // 严格成立，于是"due 为什么整体前移"只剩两个可能：钟比墙快，或者帧比 pts 到得早。
    out << "    \"arrive_mono_span_ns\": " << pace.arrive_mono_span_ns << ",\n";
    out << "    \"elapsed_span_ns\": " << pace.elapsed_span_ns << ",\n";
    out << "    \"due_span_ns\": " << pace.due_span_ns << ",\n";
    out << "    \"pts_span_ns\": " << pace.pts_span_ns << ",\n";
    // 接线自证：小于 `presented` 时上面四格全是 0，那个 0 不可读作"没动"。
    out << "    \"clock_supplied_frames\": " << pace.clock_supplied_frames << ",\n";
    out << "    \"frame_duration_ns\": " << pace.frame_duration_ns << ",\n";
    out << "    \"derived_fps\": " << fixed6(pace.derived_fps) << ",\n";
    out << "    \"lateness_ns\": {\"samples\": " << pace.latency_samples << ", \"p50\": "
        << pace.lateness.percentile_ns(0.50) << ", \"p99\": " << pace.lateness.percentile_ns(0.99)
        << ", \"max\": " << pace.lateness.max_ns() << "},\n";
    out << "    \"schedule_lead_ns\": {\"samples\": " << pace.scheduled_samples << ", \"p50\": "
        << pace.schedule_lead.percentile_ns(0.50) << ", \"spread\": "
        << (pace.schedule_lead.percentile_ns(0.90) - pace.schedule_lead.percentile_ns(0.10))
        << "},\n";
    out << "    \"clock_health\": {\"extrapolated_frames\": " << pace.extrapolated_frames
        << ", \"free_run_frames\": " << pace.free_run_frames << ", \"jump_rebases\": "
        << pace.jump_rebases << ", \"monotonic_rejects\": " << pace.monotonic_rejects
        << ", \"drift_rebases\": " << pace.drift_rebases
        // P13-AC：修正的**幅度**。只数次数分不出"一帧削平"和"二十三帧摊平" ——
        // 前者（P13-AB）单帧 45 ms，于是修一次就变成一次 29–31 ms 的迟到尖峰；
        // 后者单帧 ≤ 2 ms，把同样的修正摊到约 0.38 s 里。
        << ", \"drift_max_correction_ns\": " << pace.drift_max_correction_ns
        << ", \"drift_corrected_total_ns\": " << pace.drift_corrected_total_ns << "},\n";
    out << "    \"clock_mix\": {\"primary_frames\": " << pace.clock_primary_frames
        << ", \"fallback_frames\": " << pace.clock_fallback_frames << ", \"system_frames\": "
        << pace.clock_system_frames << "}\n";
    out << "  },\n";

    // ── 音频侧 ──
    out << "  \"audio\": {\n";
    out << "    \"opened\": " << (audio.opened ? "true" : "false") << ",\n";
    out << "    \"started\": " << (audio.started ? "true" : "false") << ",\n";
    out << "    \"negotiated\": {\"sample_rate\": " << audio.negotiated_sample_rate
        << ", \"channel_count\": " << audio.negotiated_channel_count << ", \"format\": "
        << quoted(audio.negotiated_format) << ", \"sharing\": " << quoted(audio.sharing_mode)
        << ", \"performance\": " << quoted(audio.performance_mode) << ", \"frames_per_burst\": "
        << audio.frames_per_burst << "},\n";
    out << "    \"output_format\": {\"sample_rate\": " << audio.output_format.sample_rate
        << ", \"channel_count\": " << audio.output_format.channel_count << ", \"encoding\": "
        << quoted(std::string(core::audio::pcm_encoding_name(audio.output_format.encoding)))
        << ", \"diverged\": " << (audio.output_format_diverged ? "true" : "false") << "},\n";
    out << "    \"ring_capacity_frames\": " << audio.ring_capacity_frames << ",\n";
    out << "    \"prefill_reached\": " << (audio.prefill_reached ? "true" : "false") << ",\n";
    out << "    \"startup_millis\": " << audio.startup_millis << ",\n";
    out << "    \"clock_start_ns\": " << audio.clock_start_ns << ",\n";
    out << "    \"clock_end_ns\": " << audio.clock_end_ns << ",\n";
    out << "    \"feed\": {\"packets_submitted\": " << audio.feed.packets_submitted
        << ", \"chunks_received\": " << audio.feed.chunks_received << ", \"frames_decoded\": "
        << audio.feed.frames_decoded << ", \"frames_written\": " << audio.feed.frames_written
        << ", \"frames_dropped\": " << audio.feed.frames_dropped << ", \"ring_full_waits\": "
        << audio.feed.ring_full_waits << ", \"reached_eos\": "
        << (audio.feed.reached_eos ? "true" : "false") << "},\n";
    out << "    \"output\": {\"callbacks\": " << audio.output.callbacks
        << ", \"frames_rendered\": " << audio.output.frames_rendered
        << ", \"frames_silenced\": " << audio.output.frames_silenced
        << ", \"silence_head_frames\": " << audio.output.silence_head_frames
        << ", \"silence_tail_frames\": " << audio.output.silence_tail_frames
        << ", \"first_audible_frame\": " << audio.output.first_audible_frame
        << ", \"xrun_count\": " << audio.output.xrun_count << ", \"wall_millis\": "
        << audio.output.wall_millis << "},\n";
    out << "    \"clock\": {\"queries\": " << audio.clock.queries << ", \"published_timestamp\": "
        << audio.clock.published_timestamp << ", \"published_frames_read\": "
        << audio.clock.published_frames_read << ", \"published_frames_written\": "
        << audio.clock.published_frames_written << ", \"published_frozen\": "
        << audio.clock.published_frozen << ", \"timestamp_not_started\": "
        << audio.clock.timestamp_not_started << ", \"timestamp_fail\": "
        << audio.clock.timestamp_fail << ", \"monotonic_rejects\": "
        << audio.clock.monotonic_rejects << ", \"jumps_clamped\": " << audio.clock.jumps_clamped
        << ", \"latency_frames\": " << audio.clock.latency_frames;

    // P14-E：异常时刻的原始观测窗口。
    //
    // 为什么必须出版：`monotonic_rejects` 与 `jumps_clamped` 只是**计数**。
    // 一个计数为 1 的证据，说不出"设备给的位置突然倒退"还是"我们自己的换算偏了" ——
    // 这两件事的修法完全不同（前者等平台修，后者改代码）。
    // 窗口里每一行同时给 `candidate`（设备原值）与 `accepted`（守卫输出），
    // 差值就是守卫做的事，不用猜。
    out << ", \"trace\": {\"event_count\": " << audio.clock.trace.event_count << ", \"events\": [";
    for (std::int32_t event_index = 0; event_index < audio.clock.trace.event_count; ++event_index) {
        const auto& event = audio.clock.trace.events[static_cast<std::size_t>(event_index)];
        out << (event_index == 0 ? "" : ", ") << "{\"flags\": " << event.flags
            << ", \"pivot\": " << event.pivot << ", \"samples\": " << event.samples
            << ", \"points\": [";
        for (std::int32_t index = 0; index < event.samples; ++index) {
            const auto& point = event.data[static_cast<std::size_t>(index)];
            out << (index == 0 ? "" : ", ") << "{\"mono_ns\": " << point.mono_ns
                << ", \"candidate_ns\": " << point.candidate_ns
                << ", \"accepted_ns\": " << point.accepted_ns
                << ", \"kind\": " << point.kind << ", \"stale\": " << point.stale << "}";
        }
        out << "]}";
    }
    out << "]},\n";

    // P14-G：残差尾部。计数（`monotonic_rejects` / `jumps_clamped`）只说"几次"，
    // 窗口（`trace`）只说"那一次长什么样"，都不回答"离阈值还有多远"。
    // 尾部把**每一次观测**都归档，于是"再大一点会有多少"从一次罕见事件
    // 变成一条分布 —— 这是本卡唯一的目的。
    {
        const auto& tail = audio.clock.tail;
        out << "    \"tail\": {\"samples\": " << tail.samples
            << ", \"forward\": " << tail.forward << ", \"backward\": " << tail.backward
            << ", \"max_forward_ns\": " << tail.max_forward_ns
            << ", \"max_backward_ns\": " << tail.max_backward_ns
            << ", \"clamp_threshold_ns\": " << tail.clamp_threshold_ns
            << ", \"beyond_threshold\": " << tail.beyond_threshold()
            << ", \"intervals\": " << tail.intervals
            << ", \"long_gaps\": " << tail.long_gaps
            << ", \"max_mono_step_ns\": " << tail.max_mono_step_ns
            << ", \"long_candidate_steps\": " << tail.long_candidate_steps
            << ", \"max_candidate_step_ns\": " << tail.max_candidate_step_ns
            << ", \"first_mono_ns\": " << tail.first_mono_ns
            << ", \"buckets\": [";
        for (std::int32_t index = 0; index < aaudio::kClockTailBucketCount; ++index) {
            out << (index == 0 ? "" : ", ")
                << tail.buckets[static_cast<std::size_t>(index)];
        }
        out << "], \"edges_ns\": [";
        for (std::size_t index = 0; index < aaudio::kClockTailEdgesNs.size(); ++index) {
            out << (index == 0 ? "" : ", ") << aaudio::kClockTailEdgesNs[index];
        }
        out << "]}\n";
    }
    out << "  }},\n";

    out << "  \"completed\": " << (completed ? "true" : "false") << ",\n";
    out << "  \"error\": " << quoted(error) << "\n";
    out << "}\n";
    return out.str();
}

}  // namespace kivo::video::avsync
