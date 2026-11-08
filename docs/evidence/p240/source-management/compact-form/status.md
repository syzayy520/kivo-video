# 手机连接表单补测 · 2026-11-09

本轮未通过布局验收。phone-large-font.txt、phone-large-font-retry.txt 在测试等待期间被外部终止；phone-large-font-monitor.txt 为宿主启动超时，未进入字段/键盘断言。不能据此宣称表单通过，也不能认定生产页面闪退。

测试源码 SourceCompactFormInstrumentation 保留供继续修复测试启动方式。仅使用合成凭据、隔离存储和临时密钥，不连接正式服务器。K40 系统字号已恢复且最终再次读回 1.0；输入法未修改。后续仍需真实键盘、1.0/1.5 字号、单页返回与横竖屏矩阵。用户当前优先流畅影音播放，此项保持未验。
