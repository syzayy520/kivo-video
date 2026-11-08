# Emby 全库浏览（2026-11-09）

> 暂停的未交付WIP：源码曾构建通过，尚未安装或真机验收。按[资源库整体重构](../library-rebuild/plan.md)先完成统一源管理与持久保存；本稿能力需求归入L3，不能据此宣称全库浏览已完成。

替代 emby-redesign.md 中“前 60 条、本地搜索排序”的限制。每页 60 条，服务端 SearchTerm 搜索、DateCreated/SortName/ProductionYear 排序，Id 作次排序；读取 TotalRecordCount。滚动接近末尾加载下一页，不一次下载全库。

搜索输入立即清理旧结果，350ms 防抖；搜索、排序、返回、换服务器均递增请求代次，迟到响应不得覆盖当前列表。分页以服务器返回条数推进游标，按 ID 去重；空页终止分页，即使总数陈旧也不循环请求。首次失败和分页失败分别展示原因与重试，失败不推进游标。旋转保留状态，返回释放会话。取消仅保证不接纳旧结果，不声称立即断开 socket。

自然家族树：
- emby/EmbyClient.kt：协议、分页响应解析与参数编码。
- emby/library/EmbyLibraryModel.kt：列表状态、游标、防抖与请求生命周期。
- emby/EmbySessionModel.kt：认证、持有列表模型及图像缓存。
- emby/ui/EmbyLibraryScreen.kt：网格、滚动触发、加载/失败/空态。
- docs/evidence/p240/emby-library-paging/：构建及平板脱敏验收。

验收：构建通过；真机加载超过 60 条、全库搜索、清除恢复、排序、快速查询及旋转/返回；不得存储凭据或真实片单。网络解码与音频不在本卡范围。
