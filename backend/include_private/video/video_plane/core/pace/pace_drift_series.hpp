#pragma once

#include <array>
#include <cstdint>

namespace kivo::video::core::pace {

/// 抽稀后的一个点：`(媒体时刻, 该帧的 lateness)`。
///
/// 两个都是 64 位整数，装成 16 字节 —— 序列最终躺在 `std::array` 里，
/// 不碰堆（`core/pace` 全族的稳态约束）。
struct DriftPoint {
    std::int64_t media_ns = 0;
    std::int64_t lateness_ns = 0;
};

/// 「最差的那一个点」—— 分布答不出的那一半（P13-F）。
///
/// `SignedHistogram` 给得出 `max` 有多大，给不出它**在哪儿**：
/// 分布按定义丢掉顺序。而"最差那一下在片头还是片尾"决定结论完全不同
/// （冷启动可以忽略 / 片尾收尾不是播放能力问题 / 片中抢占要修）——
/// 论证见 `docs/design/backend-core/66-p13f-design.md` §一。
///
/// `index` 与 `of` 一起才构成位置：`index / (of - 1)` 是位置比。
/// 位置比**不单独存** —— 存了就是两份位置，迟早不一致。
struct DriftExtreme {
    /// false 表示这个口径一个样本都没有。此时其余字段无意义（不是"零"）。
    bool valid = false;
    /// 时间正序的第几个被接受的样本（0 基），**全场累计**。
    ///
    /// 不是窗口内序号：极值由 `note()` 累计维护（见 `max_point()`），
    /// 所以环形覆盖之后它仍然指得到那个点，`index` 也仍是"整场第几个"。
    /// 因此**不能**拿它去 `at()` 取点 —— `at()` 收的是窗口内下标。
    std::int32_t index = 0;
    /// 该口径的**累计**样本总数（= `total_` = JSON 里的 `samples`）。
    /// `valid` 为真时必定 > 0，且恒有 `index < of`。
    std::int32_t of = 0;
    /// 那一点的媒体时刻 —— 直接与 `source.duration_us` 对照就知道片头还是片尾。
    std::int64_t media_ns = 0;
    /// 极值本身。必须与同口径直方图的 `max_ns()` 逐位相等（判据 F1）。
    std::int64_t lateness_ns = 0;
};

/// 「按媒体时间抽稀 + 定长环形序列」。
///
/// 存在的理由只有一个：**让不同帧率下的分位数可比**。
/// 逐帧采样时 60fps 的样本密度是 30fps 的两倍，p99 自然更容易摸到调度抖动的尾巴，
/// 于是同一台机器、同一段 8 秒读出"60fps 更差"—— 差的是看得更清楚，不是播得更差。
/// 论证与实测见 `docs/design/backend-core/62-p13d-design.md` §一/§三。
///
/// 抽稀门按**媒体时间**（pts）而不是帧序号：8 帧在 30fps 是 267 ms、60fps 是 133 ms，
/// 按帧序号抽稀只是把密度从"随帧率变"改成"随帧率除以 8 变"，问题一点没解决。
///
/// 只有一个写入口 `note()`：判定与存储放在一起，调用方不可能"忘了先问一声"。
/// 分成 `accept()` + `add()` 两步的版本被否掉了 —— 两处判定会给两个真相。
///
/// 线程模型：非线程安全，只该在呈现线程里用（与 `PaceStats` 一致）。
class DriftSeries {
public:
    /// 采样间隔 50 ms → 20 Hz。与帧率无关是这条卡的全部意义所在。
    ///
    /// 为什么不更密：20 Hz 下 8 秒片源有 160 个点（p99 = 第 2 差的样本，粗但可比），
    /// 60 秒有 1200 个点（够回归）。再密只是更精确地复现"逐帧"这个旧问题。
    static constexpr std::int64_t kSampleIntervalNs = 50'000'000;

    /// 2048 点 × 50 ms = 102.4 s 的窗口。超出后覆盖最旧的。
    static constexpr std::int32_t kCapacity = 2048;

    /// 段内允许的最大空洞：20 格 = 1 s（P13-M）。
    ///
    /// 为什么是 1 秒：20 Hz 抽样下 20 格意味着**至少 24 个连续帧一个样本都没进来**
    /// （任何 ≥ 24 fps 的片源都会跨过 24 个格点）。帧级抖动到不了这个量级 ——
    /// 30 fps 掉一帧是 33 ms，掉三帧（100 ms）才凑够**一格**。能连续丢掉
    /// 二十多个帧的只有卡顿或时间轴跳变。
    ///
    /// 这个数只影响 `contiguous` 这一个**读数**，不触发切段。理由见 `break_segment()`
    /// 的注释：段内的空洞无法与"向前 seek"区分，拿它切段会把卡顿之前的真数据
    /// 全部作废 —— 那种损失比"多报一个洞"大得多。
    static constexpr std::int32_t kMaxContiguousGapCells = 20;

    /// 喂一帧。返回 true 表示这个样本进了序列。
    ///
    /// 判定用**绝对时间网格**，不是"距上一个被接受的点是否够久"。
    /// 后者看着等价，实测不等价：30fps 的帧距是 33.3 ms，够久的最早一帧落在 66.7 ms，
    /// 于是采样节奏退化成 15 Hz；60fps 落在 50.0 ms，还是 20 Hz。
    /// **两个帧率采到 121 : 161 个点 —— 抽稀白做了。**（单测 `t01` 会打红。）
    ///
    /// 网格版：一律取"跨过 50 ms 整数格点的第一帧"。30fps 拿到 66.7 ms 那一帧，
    /// 记的时刻仍是它的真实 pts，但下一个格点仍是 100 ms —— 误差不会累积，
    /// 20 Hz 的节奏在两个帧率上完全一致。
    ///
    /// 三条接受理由，任一成立即可：
    ///   ① 还没有任何点（首帧必须进，否则短片源可能一个点都采不到）；
    ///   ② 媒体时刻已跨过下一个格点（正常抽稀）；
    ///   ③ 媒体时刻**倒退**且没走 `break_segment()` —— 兜底，见下。
    ///
    /// **③ 是兜底，不是主判据**（P13-M）。切段的权威是 `break_segment()`：
    /// 由 `FramePacer::reset()` 在锚点重建的那一刻调用，精确、不需要阈值。
    /// 这里保留 ③ 只有一个理由 —— pts 倒退**必然**意味着时间轴变了，那是一条
    /// 成立的充分条件。而它的逆命题从来不成立：pts 前进推不出"时间轴没变"，
    /// 因为向前 seek 就是"前进但换了位置"。P13-L 的缺陷证据正是栽在这上面：
    /// 30 s 处向前跳到 40 s，`note()` 只看到"前进"，两段被拼进同一个回归，
    /// 报出 `segments = 1` / `span = 112.367 s` / `observed_hz = 18.466`。
    ///
    /// 用存在性（倒退）去推断不存在性（没换时间轴），必然漏一次；而漏的那一次
    /// 会把两个 `lateness` 零点之间的跳变拟合成"漂移速率"。
    ///
    /// ③ 触发时做三件事：关段、清空序列（旧时间轴的点对新回归是毒药）、`segments_++`。
    bool note(std::int64_t media_ns, std::int64_t lateness_ns) noexcept;

    /// 一次**锚点重建**：关掉当前段、清空环形、段数 +1。
    ///
    /// 谁调用：`FramePacer::reset()`（`tracker_.reset()` 的同一处）。那里是
    /// `lateness = present_at − due_at` 的**零点被重新定义**的地方 ——
    /// 锚点没了，同一个 lateness 值代表的就不再是同一件事。
    ///
    /// 为什么段边界要由事件定、而不是由 pts 推断：
    /// pts 的走向只在一个方向上携带信息。倒退 ⇒ 换了时间轴（必真）；
    /// 前进 ⇒ **可能**是同一时间轴上的空洞（卡顿），也**可能**是跳到了新位置
    /// （向前 seek）。两种在 pts 上长得一模一样，那时切段会把卡顿之前的
    /// 真数据全部作废，不切段又会把两次锚定拼成一段。既然两种都错，
    /// 就不能靠 pts 决定 —— 要问那个**确实知道**的地方。
    ///
    /// 空会话上是**空操作**：没有东西可关，也不该把 `segments` 加成
    /// "发生过切段"的假数 —— 每个会话的 `start()` 都会走一次 `FramePacer::reset()`，
    /// 那一次不能让每份证据都从 `segments = 2` 起步。
    void break_segment() noexcept;

    /// 清空序列（保留累计计数与段数）。给"换片源"这种整段重来用。
    ///
    /// 同时**关掉当前段**：把它的时间轴跨度折进 `closed_span_ns_`（P13-H）。
    /// 之后从哪一点接着喂都算新段，`media_span_ns()` 不会把两段的媒体时刻相减。
    void clear() noexcept;

    [[nodiscard]] std::int32_t size() const noexcept { return size_; }
    [[nodiscard]] std::int64_t total_samples() const noexcept { return total_; }

    /// **本段时间轴内**被接受的样本数（seek / `clear()` 之后从 0 重新起算）。
    ///
    /// 与 `total_samples()` 是两个问题：那个是整场累计（跨段相加），
    /// 这个是"当前这条时间轴上采到了几个"。`size()` 只可能等于
    /// `min(本函数, kCapacity)` —— 环形的容量是**每段**的，不是整场的。
    ///
    /// 为什么要单独有一个：`truncated()` 上一版拿整场累计数去比容量，
    /// seek 之后累计数越过 2048 就谎报"环形覆盖发生过"，
    /// 而当前段其实一个点都没被覆盖。判据 I3 就是拿 `本函数 > size()` 守这个。
    [[nodiscard]] std::int64_t segment_samples() const noexcept { return segment_total_; }

    [[nodiscard]] std::int64_t observed_frames() const noexcept { return observed_; }

    /// 被抽稀门拒掉的帧数。`observed_frames = total_samples + skipped_frames`。
    [[nodiscard]] std::int64_t skipped_frames() const noexcept { return skipped_; }

    /// **没有帧跨过的格点数**（播放出现空洞、或一帧跨了好几格）。
    ///
    /// 为什么不补样本：补等于编数据。但也不能假装没发生 ——
    /// 一格没采到时，20 Hz 的实际节奏就掉了，趋势的横坐标也跟着稀。
    /// 这个数让"样本数比预期少"有一个可核对的解释。
    [[nodiscard]] std::int64_t unobserved_cells() const noexcept { return unobserved_; }

    /// 段内相邻两个被接受样本之间**最宽**的空洞，单位是格点（50 ms，P13-M）。
    ///
    /// 为什么段内还会有空洞：真卡顿。它与向前 seek 在 pts 上长得一模一样，
    /// 所以不能靠它切段（见 `break_segment()`）；能做的只有把它**出版**，
    /// 让读者知道这条回归横跨了一个洞。
    ///
    /// 与 `unobserved_cells()` 是两个问题：那个是**整场累计**（各段相加），
    /// 这个是**当前段的最大值**。累计值答不出"最宽的那个洞有多宽"，
    /// 而回归的可信度取决于最大值，不是总和 —— 十个 100 ms 的小洞不会让
    /// 一条直线失去意义，一个 10 s 的大洞会。
    [[nodiscard]] std::int32_t largest_gap_cells() const noexcept { return largest_gap_cells_; }

    /// 当前段内没有超过 `kMaxContiguousGapCells` 的空洞。
    ///
    /// 为 false 时 `media_span_ns()` 与由它算出的 `observed_hz` **包含**那个洞
    /// （跨度按"末 − 首"，洞落在里面），所以那两个数不是"实际走过的时长"。
    /// 这条限制写在字段旁边而不是只写进文档：读者看到 `false` 就知道要打折看，
    /// 而不必先去查一篇设计稿。
    [[nodiscard]] bool contiguous() const noexcept {
        return largest_gap_cells_ <= kMaxContiguousGapCells;
    }

    /// **本段时间轴内**是否发生过环形覆盖 → 序列只剩这条时间轴的最近 102.4 s。
    ///
    /// 判据是 `segment_total_ > kCapacity`，**不是** `total_ > kCapacity`。
    /// 上一版用的是后者，于是在 seek 之后说了一句没发生的事：`clear()` 把
    /// `size_` 清了 0，却没有清 `total_`，于是"整场 1600 + 1800 = 3400 > 2048"
    /// 被判成"覆盖发生过"—— 而当前段只有 1800 个点，一个都没被覆盖。
    /// 那份 JSON 会同时写着 `truncated: true` 和 `window_samples: 1800`，
    /// 自相矛盾（覆盖过的环形序列必满 2048）。见 `72-p13i-design.md` §一。
    ///
    /// 覆盖与"跨段"是两件不同的事，必须由两个不同的字段回答：
    /// 覆盖 → `truncated`；跨段 → `segments`。把它们混成一个，读者就无法解释
    /// "为什么窗口里的点比累计的少"—— 而那正是这份数据最容易被读错的地方。
    [[nodiscard]] bool truncated() const noexcept { return segment_total_ > kCapacity; }

    /// **整场**（各段时间轴跨度之和）媒体时间跨了多久（P13-H）。
    ///
    /// 与 `at(0)` 到 `at(n-1)` 的差不是一回事：后者是**窗口**跨度，
    /// 封顶 2047 × 50 ms = 102.35 s；这个数只增不减。
    ///
    /// 存在的理由是一条除法：`observed_hz = total_samples / media_span`。
    /// 分子是累计的，分母就必须也是累计的。上一版拿窗口跨度去除累计样本数，
    /// 102.4 秒之后商随播放时长线性增长 —— 130 s 报 25.4 Hz，600 s 报 117 Hz，
    /// 旁边还写着 `target_hz: 20`。**那个数没有任何上界，也不会有任何判据拦它**，
    /// 因为校验器复算的是同一个除法（见 `70-p13h-design.md` §一）。
    ///
    /// 按段累加而不是 `末 - 首`：seek 之后媒体时刻倒退，
    /// `末 - 首` 会得到负数或把两段误差算进去的假跨度。
    /// 各段各算各的再加起来，才与"各段各算各的再加起来"的 `total_samples()` 同口径。
    [[nodiscard]] std::int64_t media_span_ns() const noexcept;

    /// 时间轴段数 —— **一次锚点生命期算一段**（P13-M 收窄了定义）。
    ///
    /// 旧定义是"媒体时刻倒退的次数 + 1"，那是从 pts 推断出来的；现在由
    /// `break_segment()` 记，触发它的是 `FramePacer::reset()`（真事件）。
    /// 两者在真 seek 上一致，差异在两处，都是新定义更对：
    ///   · `stop()` 之后再 `start()` 现在也会 +1 —— 锚点确实重建了，
    ///     旧的 lateness 零点已经不存在；
    ///   · pts 倒退但没走 pacer（只在单测里发生）仍会 +1 —— 兜底保留。
    ///
    /// 为什么单独计数：`segments > 1` 时序列里剩下的只是**最后一段**，
    /// 拿它去回答"整场播放漂了多少"是错的。这个数让读者立刻知道别这么读。
    [[nodiscard]] std::int32_t segments() const noexcept { return segments_; }

    /// 按**时间正序**取第 i 个点，i ∈ [0, size())。越界返回空点。
    /// 趋势计算不该自己算环形下标 —— 那是这个类的事。
    [[nodiscard]] DriftPoint at(std::int32_t index) const noexcept;

    /// 序列里 `lateness_ns` 最大的那个点，附位置（P13-F）。
    ///
    /// 只有这个类能回答这个问题：它拥有序列，而直方图只有分布。
    ///
    /// **口径必须与 `SignedHistogram::max_ns()` 一致：有符号最大值，不是绝对值。**
    /// 漂移可以为负（视频 pts 超前音频）。若这里取绝对值，而直方图取有符号最大，
    /// 一条 −90 ms 的样本会让两边分家 —— 判据 F1 就是拿来抓这个的。
    ///
    /// 极值在 `note()` 里**累计**维护（O(1)），不在调用时扫序列。
    /// 扫序列在环形覆盖后会与直方图分家：直方图累计，序列只留最近 102.4 s，
    /// 最大值若发生在旧段就找不回来了。累计维护让两边永远同口径。
    ///
    /// 因此返回的 `index` 是**全场**第几个样本，**不能**拿去 `at()` 取点
    /// （`at()` 收的是窗口内下标）。`of` 同步是全场累计数。
    [[nodiscard]] DriftExtreme max_point() const noexcept;

private:
    /// 起一个新段：首点即该段起点，格点从它往后数。
    /// ① 首帧与 ③ 换时间轴都要做这一套，写两遍迟早少写一行。
    ///
    /// **不在这里清 `segment_total_`** —— 它的唯一写入点是 `clear()`
    /// （环形清空，计数跟着归零）。进入本函数的两条路都先过那里：
    /// 新建对象走默认值 0，seek 分支先 `clear()`。两处写同一个数早晚不一致。
    void begin_segment(std::int64_t media_ns) noexcept;

    /// 关掉当前段：把 `末 − 首` 折进 `closed_span_ns_`。
    /// 没有段可关（`has_last_ == false`）时什么也不做。
    void close_segment() noexcept;

    std::array<DriftPoint, kCapacity> points_{};
    std::int32_t size_ = 0;
    std::int32_t write_index_ = 0;  ///< 下一个写入位；`size_ == kCapacity` 时即最旧的点
    std::int64_t total_ = 0;        ///< 被接受的累计点数（含已被覆盖的，跨段相加）
    /// 当前段内被接受的样本数（P13-I）。`begin_segment()` 归零。
    ///
    /// 与 `total_` 分开的理由只有一个：**环形的容量是按段算的**。
    /// 拿跨段相加的数去比容量，seek 之后必然谎报覆盖（见 `truncated()`）。
    std::int64_t segment_total_ = 0;
    std::int64_t observed_ = 0;     ///< 喂进来的帧总数
    std::int64_t skipped_ = 0;      ///< 被抽稀门拒掉的帧数
    std::int64_t unobserved_ = 0;   ///< 没有帧跨过的格点数（整场累计）
    /// 当前段内最宽的空洞（格点，P13-M）。`clear()` 归零，与 `segment_total_` 同处。
    std::int32_t largest_gap_cells_ = 0;
    std::int32_t segments_ = 1;     ///< 时间轴段数（seek 会 +1）
    bool has_last_ = false;
    std::int64_t last_media_ns_ = 0;
    std::int64_t next_due_ns_ = 0;  ///< 下一个待采格点（绝对网格，误差不累积）
    /// 已结束各段的时间轴跨度之和（P13-H）。`clear()` 不清它，与 `total_` 同理。
    std::int64_t closed_span_ns_ = 0;
    /// 当前段的第一个媒体时刻。`has_last_ == false` 时无意义。
    std::int64_t segment_first_media_ns_ = 0;
    /// 当前段**最后被接受**的那一帧的媒体时刻（不是最后观察到的那一帧）。
    ///
    /// 两者差的是"尾部被抽稀门拒掉的那几帧"。跨度用接受的：被拒的帧没有样本，
    /// 把它们算进跨度，等于把没采到的时间算成采到了 —— 而且会让累计跨度
    /// 比窗口跨度大不到一帧，两根尺子差一点点比差很多更难发现（单测 t04 会红）。
    std::int64_t segment_last_accepted_ns_ = 0;
    /// 累计极值点（P13-F）。`clear()` 不清它，与 `total_` 同理。
    DriftExtreme worst_{};
};

}  // namespace kivo::video::core::pace
