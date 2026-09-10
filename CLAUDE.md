# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目是什么

医院门诊/值班管理桌面客户端（Qt6 Widgets + qmake），与服务端通过自定义 TCP 二进制协议通信。不是完整单体：`hos.pro` 编译出的 `hos.exe`（输出到 `bin/`）只包含客户端 UI + 网络逻辑，所有业务数据由远端服务端(192.168.133.131:10086)下发。

## 编译 / 运行约定（重要）

- **不要运行编译或启动程序**。qmake/nmake 与真机运行由用户本人执行（Qt 6.10.3，MSVC 套件，工程由 Qt Creator 打开 `hos.pro`）。如确需验证改动，只做静态检查、给出改动说明，交用户编译。
- 工程要的 Qt 模块在 `hos.pro` 第 1 行：`core gui widgets network serialport webenginewidgets`，套件必须带 WebEngine。
- `DESTDIR = $$PWD/bin`，可执行文件落在 `bin/hos.exe`。
- `CONFIG += c++17`。
- 命令行构建（仅参考，用户自己跑）：`qmake hos.pro` 后用 nmake/jom 编译。`build/` 是 Qt Creator 的影子构建目录。Qt Creator 的 Debug 构建 = `build/<kit>/Makefile.Debug` + `debug/*.obj`，moc 产物在同级 `debug/moc_*.cpp`。
- **给头文件新加（或新引入）`Q_OBJECT` 后必须重跑 qmake**（Qt Creator：构建 → 执行 qmake）。qmake 的 AUTOMOC 在 **qmake 阶段**扫源码文本才决定给哪些头生成 moc 规则；只改 .h 不重跑 qmake 就不会生成 `moc_xxx.cpp`，链接必报 `LNK2001 无法解析的外部符号 ...::metaObject`。2026-09-10 `Tool/recordpdf.*` 从普通类改成 `QObject` 时踩过一次，别把它当代码错去查。

## 启动流程与线程模型（关键）

1. `main.cpp`：`CData::init()` 读配置 → `new SocketLink` → `connectHost()`（**此时 SocketLink 在主线程**）→ 显示 `LoginWidget`。
2. 登录时 LoginWidget 直接（同步调用）`m_socket->send_data(...)` 发登录包 —— 此时 socket 还在主线程，直接调用是安全的。
3. `socketlink.cpp` 收包解析出 `resp.role` 写入 `CData::m_role`，`login_success` 信号让 main.cpp 的 lambda 决定窗口：`m_role==0`（普通用户/医生）→ `MainWindow`，`m_role==1`（管理员）→ `ManngerWindow`。
4. **两窗口构造函数里**会 `QThread *thread=new QThread; m_socket->moveToThread(thread); thread->start();` —— 之后 SocketLink 全部收发解析都在这个 worker 线程跑。
5. 因此规则：**窗口/UI 只能通过信号连接到 `SocketLink`（自动队列，跨线程安全），不要在 worker 线程 move 之后再从 UI 线程直接调用 `m_socket` 的方法**。LoginWidget 是特例（move 之前直连）。
6. 收包任务在 worker 线程把数据写进 `CData` 静态量，再由 `*_success` 信号（自动 queued）切回主线程调用 UI 的 `flush()/flush_table()` 刷新。可见性靠 queued 信号保证顺序；若两个响应紧挨着到达，第二个解析可能覆盖静态量（见“已知问题”）。

## 核心架构：一套“信号上行 + CData 中转 + 信号下行”的请求流

UI 各 pane **不持有 socket**，也不发请求函数。唯一数据通路是：

```
pane 组装请求字节(HEAD+packed struct，memcpy 进 1024 字节 QByteArray)
  → 发信号(如 AppointWidget::to_get_meet / GuardWidget::send_my_data)
  → MainWindow/ManngerWindow::init_task_connect() 里 connect 到 SocketLink::send_data
  → 服务端回包 → SocketLink::recv_data() 解析 HEAD，按 head.type 分发
  → new 对应 Task → execute() 把 body 拷成 protecol.h 里的 struct 并写入 CData 静态量 → delete task
  → emit SocketLink::*_success
  → 窗口里 connect 该信号到对应 pane 的 flush()/flush_table()（主线程刷新 UI）
```

**新增一类服务，需要同步改的地方**：
1. `MyTcp/protecol.h`：`SERVICE_TYPE` 枚举加类型；按服务端字节布局加 REQ/RESP struct（注意对齐/packing）。
2. 请求方（某 pane）：用 memcpy 手工拼 `HEAD` + REQ 到 QByteArray（`HEAD.len` = body 字节数），emit 一个信号。
3. `MyTcp/socketlink.h/.cpp`：加一个 `*_success` 信号；`recv_data()` 里 `else if(head.type==...)` 加分支 `new XxxTask(head.len, recv_data)->execute(); emit xxx_success(); delete task;`。
4. `Task/` 下加 `XxxTask` 子类（继承 `BusinessTask`）：`execute()` 里把 `this->data` memcpy 成 RESP struct 写 CData。
5. 窗口 `init_task_connect()` 里 connect 上行信号→`send_data`、下行 `*_success`→pane 的刷新槽。

`SERVICE_TYPE` 与 `recv_data()` 现有分发映射：

| head.type | Task | 写入的 CData |
|---|---|---|
| `DOCTOR_LOGIN` | LoginTask | `m_id`, `m_role` |
| `DOCTOR_APP_INFO` | GetInfoTask | `app_info` |
| `SELECT_DOCTOR` | GetDepartmentDoctorTask | `selece_department_info` |
| `GET_GUARD` | GetGuardTask | `m_get_cards`（3×7 排班矩阵） |

发送侧另有一个 `ToLoginTask`（loginwidget 里手动 new+execute 得到字节再 send），不属于收包分发。

## 目录与分层

- `MyTcp/` —— 网络层。
  - `protecol.h`：全部线上 struct。`HEAD` 24B（非 packed）；`GUARD_REPIX_T`/`GET_GUARD_RESP` 在 `#pragma pack(push,1)` 内是字节对齐的；其余默认对齐。`DOCTOR_APP_RESP` 因 `char[10][15]*3` 后接 `int state[10]` 有隐藏 2 字节填充（sizeof==500），千万别按“字段加起来 498”去算长度。
  - `socketlink.{h,cpp}`：TCP 收发、粘包缓冲（`buf_data[4096]` + `p_use/p_now`，满时用 `MyUtils::ruleBuf` 压缩）、`recv_data()` 分发。只处理一个完整包，**收到后若 `p_use!=p_now` 直接把缓冲清零（注释“最终兜底”），多余字节被丢弃**——这是概率性丢包的根源之一。
  - `cdata.{h,cpp}`：`CData` 全局静态数据仓（ip/port/尺寸/登录态/各类缓存）+ 配置读取。`readJsonFile()` 从 qrc `:/configs/total_config.josn` 读 `ip/port/CURRENT_SIZE`。
- `Task/` —— `BusinessTask` 子类。**functor 式**：无父对象，调用方 `new→execute()→delete`，同步在 socket 线程执行，不是 QThread/异步。基类只存 `int len; QByteArray data;`，虚函数 `execute()`；`is_success` 只在 LoginTask 上有。
- `pans/` —— 四类业务页 + 登录页。
  - `loginwidget`：账号/密码登录（手机+验证码 tab 只做了 UI，功能未实现；验证码框文本收了但没验证没上包）。
  - `syswidget`：首页启动台（6 个图标按钮中 查看预约/值班信息/工作统计 接了信号到各页，其余占位）。
  - `appointwidget`：按 今天/本周/本月 查预约 → `MedicalCardWidget` 列表（2 列）。**点击某张预约卡（卡片 `cardClicked` 信号）→ 弹出接诊详情弹窗 `AppointDetailWidget`**（`openMeetDetail(idx)`，idx=该卡在 `CData::app_info` 的下标），医生在该弹窗录入 诊断/处方，点“完成”→警告二次确认 → qDebug 打印本次预约全量信息并 **accept() 后由 `sendRecord()` 打包 `HEAD(type=DOCTOR_SET_RECORD)+SET_RECORD_REQ`{meet_id/doctor_id/patient_id/char diagnosis[200]/char treat_plan[200]}，经同一 `to_get_meet` 信号发出**（复用 getAppInfo 的打包规范：head.len=sizeof(body)、data.resize(HEAD+body)、memcpy 拼包；诊断/处方用 copyCStr 拷 UTF-8 进 char[] 带结束符）。`rec.doctor_id` 取 `CData::m_id`，`rec.patient_id` 取 `app_info.patient_id`（服务端 DOCTOR_APP_RESP 已下发 patient_id[i]）。发送是 fire-and-forget（recv_data 对 DOCTOR_SET_RECORD 无回包分支，同 REPIX_GUARD）。
  - `guardwidget`：只读值班表（`CustomCard` 3×7 网格 + 本周日期表头）。
  - `orderwidget`：值班编排/保存（科室→医生下拉、点格子标记修改、保存复用 `get_doctor_info` 信号发 `REPIX_GUARD`）。
  - `doctororder`（管理员专用，`ManngerWindow` 唯一业务页）：医生×一周 排班表。布局三区：**左列医生名固定**（不随滚动滑走，纵向与表格行同步）、顶部**固定时间栏**（周一~周日日期，横向与表格联动）、右下表格(横/纵向滚动)。横轴=7 天、每天块内含 上午/下午/晚上 三张无间距 `DoctorSlotCard`（单 label 显示时段，92×40 固定）；行高、左列边距、时间栏宽靠文件顶部 kNameW/kCardW/kRowH/kGap/kTimeBarH 常量对齐。工具条 上一周|选择科室|保存修改|批量复制排班|智能排班|...|下一周。**选科室 → 发 `SELECT_DOCTOR` 填充医生名，`msleep(100)` 后发 `GET_GUARD`(本周排班)**；排班按医生行转置显示（值班时段蓝底 + “科室/坐诊”）；上一周/下一周若已选科室会重查该周排班。**点击卡片 = 指派/取消该行医生值班**；改动用“每行医生 21 位修改标记 `m_modified`（位置= d*3+k）”记录，只要点过就置位、永不因改回而复位。**保存按钮**：打印修改项后，参照 orderWidget 把(天,时段)中被改动过的槽位最新记录打包 `HEAD(type=REPIX_GUARD)+GUARD_REPIX_T*` 发出（`save_guard_info` 信号）。**批量复制排班**（m_copyBtn）：弹 `CopyScheduleDialog`（见 childs），选“复制到未来连续 N 周”（实时显示 源周/下一周/末周 日期），点确认先弹二级确认（提示整周覆盖），确认后 `startCopyWeeks` 把当前展示周**整周**（含空档 isfree=true，即整周覆盖语义）逐周各打一个 `REPIX_GUARD` 包进 `m_copyQueue`，由 `m_copyTimer`(250ms) 节奏逐包 `save_guard_info` 发出（代替连包 sleep，界面不冻结），发完恢复按钮。**注意：服务器 GET_GUARD 下发的值班槽 id 常为 0（界面只按 name 显示；行医生 id 由 SELECT_DOCTOR 下发到 `m_docIds`）**，复制采用**行驱动直取 id**：先把每个值班格归属到其医生行(owner 表，同一源周只解析一次)，打包时 `id=m_docIds[行]` 直接取，不做“按名字反查”；值班医生在医生表找不到对应行、或两行医生同名(重名无法区分)时该格**跳过不发**（宁缺毋滥，绝不发 id=0 的值班，目标周该格保持原样）；空档发 isfree=true 占位清除。每包 `head.len/frag_total` 按实际装入条数算，字节按实际长度截断(勿带尾部 0)。打包前打印“批量复制警告/汇总”便于排查。**智能排班**（m_smartBtn）：弹 `SmartScheduleDialog`（见 childs），进入即默认当前展示周、**不再发任何服务器请求**（周/科室/医生/现有排班全由 DoctorOrder 快照传入）；两种模式 覆盖当前已排班(整周重排)/保留当前已排班(只补空格，已排班次计入公平)；约束可叠加：整日禁排(某天全禁)、某医生某星期几禁排、每人每天最多班次(默认1)；默认尽量排满、每时段一人。点“生成方案”→预览(3×7 + 各医生班次数)；点“应用并保存”→确认后 `accept()`，DoctorOrder::applySmartPlan 按与旧排班的差异打包一次 `REPIX_GUARD` 发出、清空 m_modified 并刷新卡片。算法在 smartplan 模块封装（医生序号空间），`runPlan()` 按算法 id 分发（现仅 AlgBalancedWeek“本周尽量平均”：最难排的格先排、选已排班次最少的医生，可复现），便于后续加新算法。无医生时表格区显示提示。
  - `workstatwidget`（用户窗“工作统计”页，pans/workstatwidget）：一行 4 张指标卡（今日访问量/本月访问量/本月出勤天数/加班次数），`seed()` 按 `CData::is_check` 随机填数值或显示 “--”。纯装饰页，不发网络请求。
  - `recordwidget`（用户窗“查看病例/病历”页，pans/recordwidget，代码式 UI 无 .ui）：**只有界面与信号槽，数据通信未接**。三段布局——查询条（姓名 `m_nameEdit` + 就诊日期起止两个 `QDateEdit`（默认本月1日~今天；三个日期框统一走 `makeDateEdit()`，`setButtonSymbols(QAbstractSpinBox::NoButtons)` **去掉下拉/微调箭头**，外观与普通输入框一致，日期靠键入 yyyy-MM-dd）+ 查询/重置）、左侧结果表（就诊日期|姓名|性别|年龄|诊断，`SelectRows` 单选，只读）、右侧可编辑详情表单（就诊日期/姓名/性别 QComboBox/年龄 QSpinBox/接诊医生/诊断 QTextEdit/治疗意见 QTextEdit + 保存修改/撤销修改）。**表格行 ≠ 数据下标**：每行第 0 列的 `Qt::UserRole` 存它在 `m_rows` 里的下标（`kRowIndexRole`），查询过滤后靠这个映射回 `m_rows`。本地联动：查询=按姓名 contains + 日期闭区间过滤 `m_rows` 重刷；重置=清条件回本月；选中行→表单加载（换行前有未保存改动会弹“是否放弃”并回退选中）；保存=读表单写回 `m_rows[idx]` + 重刷 + qDebug（记录的新日期若跳出查询范围会提示“列表里暂时看不到”）；撤销=从数据行重灌表单。脏判断拆成 `formDirty()`（带 `m_loading` 闸门，供交互用）与 `formDiffersFromRow()`（纯比较，供 `refreshTable()` 用——刷新时表单有未保存改动就**不重灌**，别让一次查询把医生敲的字吞掉）。**数据源是 `seed()` 的占位数据**（`CData::is_check` 为 true 才有，同工作统计页约定；日期都造在本月内以便默认查询可见），接入服务端时删掉 seed、改由 `flush_table()` 从 CData 灌。预留给通信的两个上行信号 `to_query_record()/to_save_record()` **目前无接收方且不带 QByteArray 入参**，落点见 `mainwindow.cpp::init_task_connect` 里的 TODO 注释。
  - `mannger/managerstatwidget`（管理员“工作统计”页，pans/mannger/ 下，文件内自带 `TrendChartWidget` 自绘折线）：内部 QStackedWidget 两个视图——**访问量趋势页**（顶部 日/周/月/年 QButtonGroup 分段按钮切换刻度 → `onScaleChanged` 重新随机生成折线数据）与 **医生工作量表页**（右上“医生工作量/查看趋势”按钮来回切；QTableWidget 表头 医生|月出勤天数|月访问量|月加班次数，一行一位医生）。折线图 QPainter 手绘：白底圆角卡片 + 渐变面积 + #2F80ED 折线/点，X/Y 刻度自适应；`CData::is_check==false` 时留空并显示“暂无数据”。
  - `childs/`：`customcard`(排班格子，通用可复用)、`selbtn`(今天/本周/本月分段按钮)、`medicalcardwidget`(预约卡；可点击，左键点整卡发 `cardClicked()` 信号)、`doctor/`(普通医生端弹窗，现仅 `appointdetailwidget.{h,cpp}` = 接诊详情弹窗：入参 `MeetRecord`{meet_id/doctor_id/patient_id/doctor_name/patient_name/time} 快照，诊断/处方两个 QTextEdit，左右布局、左侧保持原貌、右侧新增舌苔图片位 `QLabel` m_tongueLabel（暂无图片默认黑底占位，后续接图 setPixmap），返回=reject、完成=QMessageBox::warning 二次确认后 `printAll()` qDebug 打印全量信息再 accept；对外暴露 `diagnosis()/treatPlan()` 供调用方读文本打包 DOCTOR_SET_RECORD；**底部左侧另有“一键导出”开关按钮**（`m_exportBtn`，`setCheckable(true)`，**只用黑白**：未开=白底黑框黑字“一键导出”、开启=黑底白字“取消”，靠 QSS `:checked` 黑白反色 + toggled 槽 `updateExportBtn()` 改文字，再点恢复；开启状态由 `exportPdfEnabled()` 对外暴露）；开启时点“完成”→二次确认→`printAll()`→`startExport()`：**导出是异步的**，此路径**不在 onDone 里立刻 accept()**，而是禁用三个按钮 + `m_exporting=true` + 完成键文字改“导出中…”，把 `RecordPdfData` 交给挂在弹窗下的 `RecordPdf`（`exportRecord()`），等 `RecordPdf::done(path,err)` 回调里弹 QMessageBox::information（成功，报路径并存 `exportedFilePath()`）/ warning（失败，**不阻断完成就诊**，诊断处方照常上包）后再 `accept()`；导出期间重写了 `reject()`，Esc/右上角 X 一律被忽略（`m_exporting` 时只 qDebug），免得弹窗关掉、离屏页面被销毁导致导出半途而废；代码式 UI 无 .ui)、`mannger/doctorcard.{h,cpp}`(DoctorSlotCard：扁时段瓦片，管理员排班用)、`mannger/copyschedule.{h,cpp}`(CopyScheduleDialog：批量复制排班弹窗，管理员排班页 DoctorOrder 专用；选 N 周+日期显示+确认/取消+二级确认，确认后由 DoctorOrder 发包)、`mannger/smartschedule.{h,cpp}`(SmartScheduleDialog：智能排班弹窗，模式/禁排矩阵/预览/应用；与 smartplan 配合，进入不联网)、`mannger/smartplan.{h,cpp}`(纯算法模块，医生序号空间：`SmartPlanIn/Out`、`runPlan()` 按算法 id 分发、`planBalancedWeek()` 本周尽量平均)。`chatbom`、`circularavatar` 已编译但**从未被实例化（遗留死代码）**。
- `Tool/` —— `myutils.{h,cpp}`(静态样式助手 `setIcons/setBack/setLabel/setLabelImg/weekAndDate/getThisWeekMondayStr` + 缓冲压缩 `ruleBuf`)；`readutil.{h,cpp}`(读 qss 文件、去 BOM、`setWidgetQss`)；`recordpdf.{h,cpp}`(病历 PDF 导出，**QObject + 离屏 QWebEngineView 方案**（不是 QPdfWriter）：`RecordPdfData` 结构 + `RecordPdf::defaultDir()/buildFileName()/exportRecord()/exportTo()`，**全异步**——`setHtml(内存 HTML 病历页)` → `loadFinished` → `runJavaScript("fillPatientInfo(...);fillClinical(...)")` 灌数据 → `page()->printToPdf(path, QPageLayout(A4,Portrait,0边距))` → `pdfPrintingFinished(path,success)` 发 `done(filePath, err)`。**调用方必须等 `done` 信号**（runJavaScript/printToPdf 都不阻塞）。HTML 模板是 .cpp 里的 `kRecordHtml` 原始字符串（版式=成都中医药大学附属医院 门(急)诊病历，`widget.cpp` 里那份死代码 HTML 的正规化：`.paper{width:210mm;min-height:297mm;padding:25mm}`，固定区 `contenteditable="false"`、只有治疗意见/医师签名可编辑，颜色只用黑白，`@media print` 里去掉灰底与 body padding——printToPdf 走的就是 print 媒体）；JS 桥 `fillPatientInfo(name,sex,age,reg,date)` + `fillClinical(诊断,治疗意见,签名,舌苔dataURL)`（编号用 `QJsonArray`+`QJsonDocument` 拼实参，避免手工拼串注入），另留 `getEditContent()` 供以后网页直接录入用。性别/年龄无数据来源、留空；登记号=meet_id；舌苔图非空才显示该区块（PNG base64 dataURL）。输出目录 `<系统文档>/诊疗记录`，文件名 `患者名_预约号_yyyyMMdd.pdf` 重名加序号。**注意**：`setHtml` 有 2MB 上限（图大时改走临时文件）；未显示的离屏 view 也能 printToPdf；本类析构时会自己收掉 view（没有父对象）。)
- 根目录：
  - `mainwindow`(角色0 用户) 与 `manngerwindow`(角色1 管理员)**已经分道**：.ui 外壳仍相似（侧栏+堆叠页），但 `ManngerWindow` 只保留 首页 `SysWidget` + 新排班页 `DoctorOrder`，移除了 appoint/guard/order 页。改共用组件（如 SysWidget）注意两窗映射不同（见导航小节）。
  - `widget.{h,cpp}`：死代码（QWebEngineView 病历表单，main 里被注释，未展示）。`hos.pro` 仍编译它。**注意它的 HTML 已被 `Tool/recordpdf.cpp` 的 `kRecordHtml` 取代**（2026-09-10 起的病历 PDF 用的就是从那页 HTML 演化来的模板），这份 `Widget` 类现在可以删；另：它也是本工程“WebEngine 依赖已经过编译验证”的证据。
  - `data/globel.{h,cpp}`：死代码，**不在** `hos.pro` 的 SOURCES/HEADERS 里，无人引用，别动。
- `qss/tree.qss`：唯一被加载的样式，只作用于左侧导航 `QTreeWidget#treeSideMenu`（两窗口 ctor 里 `setObjectName` 后 `ReadUtil::setWidgetQss` 从 `:/qss/tree.qss` 加载）。**没有全局 QSS**，其余样式都是各控件内联 `setStyleSheet` 或 MyUtils 渐变。
- `resource.qrc`：前缀 `/`，含 icons/、configs/、qss/tree.qss。注意配置文件名是 `total_config.josn`（拼错，刻意为之，qrc 与 cdata.cpp 一致，别“顺手”改成 .json）。

## UI 约定 / 导航

- 用户窗 `MainWindow`：左侧导航 + 右侧 `QStackedWidget`（sys→workstat→appoint→guard→order→record 依次 add），默认页 sys。`itemClicked` 索引映射 **0→sys，1→record(查看病例)，2→workstat(工作统计)，3→guard，4→appoint，5→order**，6 未接。首页 `SysWidget` 图标：查看病例(toolButton)/值班信息/查看预约/工作统计 四路都接了。
- 管理员窗 `ManngerWindow`：**只 add sys→workstat(管理员统计)→DoctorOrder**，默认 sys；`itemClicked` 索引映射 **0→sys，2→workstat，3/5/6→doctor_order**（值班信息×2、值班管理→排班），其余(病例/预约)break；首页 `SysWidget` 的 值班信息/查看预约 图标切到 doctor_order，**工作统计图标切到 workstat**。
- 工作统计两窗是**两套独立装饰页**（真实统计未接）：用户窗 `WorkStatWidget`(pans/workstatwidget) 一行 4 指标卡（今日访问量/本月访问量/本月出勤天数/加班次数）；管理员窗 `ManagerStatWidget`(pans/mannger/managerstatwidget，内含自绘折线 `TrendChartWidget`) 内部 QStackedWidget 两视图——访问量趋势（顶部 日/周/月/年 分段按钮切换刻度，QPainter 画渐变面积折线）与 医生工作量表（右上“医生工作量/查看趋势”按钮来回切，表格列：医生|月出勤天数|月访问量|月加班次数）。**数据全部是预留占位：`CData::is_check==true`（main.cpp 里现赋 true）才随机填充，false 显示空/“--”，等真实统计服务接入**。
- 每次切页都写 `CData::current_widget`；pane 的 `flush_*` 常先 `if(CData::current_widget != this) return;` 防刷新到隐藏页。
- 视觉：浅蓝医疗风。页背景用 `MyUtils::setBack`(默认 `#D4E9F9→#FFFFFF`)，侧栏 `#D5E9F9`，主强调蓝 `#1E70BF/#2F80ED`，高亮 `#8AAEDE`，卡片 `#B7D4F2`。图片全走 qrc `:/...`。要做圆角背景的控件需设 `Qt::WA_StyledBackground`。

## 已知问题 / 地雷（改之前先看）

- **收包解析无长度防护**：Task 都直接 `memcpy` body 进定长 struct，信任服务端 `head.len`；`GetInfoTask` 干脆忽略 len 硬拷 500B，并按 `resp.count`(未 clamp，可能>10) 越界索引。`GetGuardTask` 用 `memcpy(&m_get_cards, this->data, ...)`，依赖 QByteArray 已废弃的隐式指针转换，应改 `this->data.constData()`。
- **同时收到两个包会概率性失败**（git commit 086da3e 已注明）。`recv_data()` 每轮只解析一个包，尾包被“最终兜底”清空丢弃。现只在 OrderWidget `to_get_guard()` 里 `QThread::msleep(100)` 阻塞缓解（代码注释“一定要改，暂且如此”，GUI 线程 msleep 是坏味道，不要新增类似 sleep）。
- **跨线程写 CData**：任务在 worker 线程写 `CData` 静态量，UI 在主线程读；若同一响应类型短时间内来两份，后一份可能覆盖前一份还没被读的静态量。
- `HEAD.msg_sn` 多数构造点未初始化（栈垃圾随包发出）。
- 登录包把密码明文 qDebug 打出（ToLoginTask）；`login_style` 硬编码 1；验证码未实现。
- `OrderWidget` 保存和获得医生**复用同一个 `get_doctor_info` 信号**（代码注释“修改与获得共用吧”），靠 `HEAD.type`(`REPIX_GUARD` vs `SELECT_DOCTOR`)区分，但 `recv_data()` 对 `REPIX_GUARD` **没有分支**——保存是否成功服务端不返回，本地只改卡片状态。
- GuardWidget/OrderWidget `flush_table()` 两侧分支都写 `isfree=true`（注释自嘲不想重构），改逻辑别照抄。
- **父页 `MyUtils::setBack` 的 `QWidget{background:qlineargradient}` 会沿父子链级联进以该页为 parent 的弹窗**（QDialog exec 也算子对象）：弹窗内没在自己 QSS 里显式设 background 的 QLabel 会透出外层蓝白渐变，与弹窗自身底色不一致。修法：在弹窗 QSS 加近层规则如 `AppointDetailWidget QLabel { background: transparent; }` 整平（例见 appointdetailwidget.cpp）。QTextEdit/按钮等自带不透明背景的控件不受影响。
- 请求/响应 struct 是**与远端 C 服务端约定的固定内存布局**，改字段=改协议，须两端一致。
- `DoctorOrder`：已接 GET_GUARD 排班显示、点击指派/取消、REPIX_GUARD 保存（打包改动槽位经 `save_guard_info`→send_data 发出，**fire-and-forget，无回包**，recv_data 对 REPIX_GUARD 无分支属正常）；改动用每行医生 21 位单调标记 `m_modified`（DocModified.flag[21]，位置=d*3+k）记录。发 GET_GUARD 前有 `QThread::msleep(100)` 临时缓解两包并发（同 orderwidget 写法，勿新增类似 sleep）。科室下拉硬编码 内科/外科（同 orderwidget）。
- 死代码清单（可安全忽略或后续删除）：`widget.*`、`data/globel.*`、`pans/childs/chatbom.*`、`pans/childs/circularavatar.*`、`MyTcp/socketlink.cpp` 里大段被注释的 `onConnected/onReadyRead` 旧逻辑。

## 给未来会话的维护约定

- **用户自己不编译**：所有改动停留在源码层面，提交/演示前由用户执行编译；不要自己跑 qmake/nmake/启动 exe。
- **每次对话都要同步更新本文件**：当本次改动触及了上面的架构、目录、协议、导航映射、新增服务流程、或新增/消除了死代码/已知问题时，对话结束时把这些变化更新回 CLAUDE.md（并在文末“变更记录”追加一行）。发现本文件与实际代码不符时，先修正本文件再动手。
- 注释与沟通多用中文，贴合现有代码习惯。

## 变更记录

- 2026-09-05：基于对全仓代码通读生成初版（覆盖网络/协议、Task、UI、工具层与已知问题）。
- 2026-09-05：管理员窗重构为 首页 SysWidget + 新排班页 `DoctorOrder`（pans/doctororder.{h,cpp}，移除 appoint/guard/order 页）；新增瓦片组件 pans/childs/mannger/doctorcard.{h,cpp}；hos.pro 已加新文件；DoctorOrder 纵轴医生走 `SELECT_DOCTOR`，实现 上一周/下一周，保存与格子内容为占位。
- 2026-09-05（同日微调）：DoctorOrder 表头改为**固定顶部时间栏**（与表格横向联动、无医生也显示）；去掉首列“医生”两字(留空占位)；DoctorSlotCard 简化为单 label 显示时段、高度降到40。
- 2026-09-05（同日再调）：DoctorOrder **医生名拆成左侧固定列**（不随滚动消失，纵向与表格滚动同步），表格仅剩 7 天列；底部加弹簧吃掉剩余高度；行高/边距与卡片用常量对齐。
- 2026-09-05（同日三调）：DoctorOrder 移植 GET_GUARD 排班查询（选科室→SELECT_DOCTOR，msleep(100) 后 GET_GUARD；翻周已选科室则重查），排班按医生行转置显示到卡片；卡片支持点击指派/取消（本地 m_info）；DoctorSlotCard 占用时蓝底白字；保存(REPIX_GUARD)仍未接。
- 2026-09-05（同日四调）：DoctorOrder 改为**每位医生 21 位单调修改标记** `m_modified`（7天*3段，点过即 true，改回也算修改）；保存按钮只打印修改过的项（医生名+id+“科室·已排/已取消”）。
- 2026-09-05（同日五调）：DoctorOrder 保存按 orderWidget 思路**发服务器**：点“保存修改”→先打印改动→按(天,时段)找出被标记改动的槽位，打包 HEAD(type=REPIX_GUARD)+若干 GUARD_REPIX_T，经新信号 `save_guard_info`→send_data 发出（无回包）。
- 2026-09-05（六调）：DoctorOrder 新增**批量复制排班**：工具条加“批量复制排班”按钮 m_copyBtn→弹新组件 pans/childs/mannger/copyschedule.{h,cpp}(CopyScheduleDialog，弹窗内 QSpinBox 选复制到未来连续 N 周并实时显示源周/下一周/末周日期，确认按钮先弹二级确认再 accept)；确认后 DoctorOrder::startCopyWeeks 把当前展示周整周 21 槽(含空档)逐周各打一个 REPIX_GUARD 包（日期改写成目标周 yyyy-MM-dd）进 m_copyQueue，由 m_copyTimer(250ms) 节奏逐包 sendNextCopyPack→save_guard_info 发出，发完恢复按钮。hos.pro 已注册新文件。
- 2026-09-05（六调·修）：批量复制曾把 GET_GUARD 下发的值班槽 id=0 原样转给服务器（历史“保存修改”只重发手动点过的槽所以没暴露）；startCopyWeeks 改为按值班医生名在当前科室表 m_docNames/m_docIds 反查真实 id 填入，查不到才退回源槽 id。
- 2026-09-05（六调·二修/方案A）：反查 id 有重名隐患，改为**行驱动直取**：startCopyWeeks 先把每个值班格归属到其医生行(owner[7][3]，同一源周只解析一次)，打包时 id 直接取该行 `m_docIds[行]`，不再按名字反查；值班医生在表里找不到对应行或两行医生同名时该格**跳过不发**(绝不含 id=0 值班)；每包 head.len/frag_total 与实际装入条数一致、字节按实际长度截断，不再固定 21 槽/1368 字节；打包前打印警告/汇总便于排查。
- 2026-09-05（七调）：DoctorOrder 新增**智能排班**：工具条加“智能排班”m_smartBtn→弹 pans/childs/mannger/smartschedule.{h,cpp}(SmartScheduleDialog：覆盖/保留两种模式、每人每天最多班次 QSpinBox(默认1)、整日禁排+医生×星期几禁排矩阵(可叠加)、点“生成方案”出 3×7 预览与各医生班次、点“应用并保存”→确认→accept)；进入时周/科室/医生/现有排班全走 DoctorOrder 快照，**不发任何服务器请求**。算法在 pans/childs/mannger/smartplan.{h,cpp}（纯医生序号空间，SmartPlanIn/Out，runPlan 按算法 id 分发，现仅 planBalancedWeek 本周尽量平均：最难排的格先排、选已排班次最少者，可复现）。应用后 DoctorOrder::applySmartPlan 按与旧排班差异只发变化的格(一次 REPIX_GUARD)，清 m_modified 并刷新。hos.pro 已注册。
- 2026-09-07：普通医生端“查看预约”新增**接诊详情弹窗**：`MedicalCardWidget` 可点击（左键发 `cardClicked()`，卡内纯展示控件已 `WA_TransparentForMouseEvents` 透传点击）；`AppointWidget` 每张卡 connect cardClicked→`openMeetDetail(idx)`，把 `CData::app_info[idx]` + `CData::m_id` 组 `MeetRecord` 快照弹 `AppointDetailWidget`（新目录 pans/childs/doctor/，代码式 UI，展示患者/时间/医生/预约号 + 诊断处方 QTextEdit，返回=reject、完成=QMessageBox::warning 二次确认后 `printAll()` qDebug 全量打印再 accept；发送逻辑见下条二调）。为让 doctor_name 有真值，`APP_INFO` 增加 `QString doctorName`，`GetInfoTask` 从 `DOCTOR_APP_RESP.DoctorName[i]` 填入。hos.pro 已注册新文件。
- 2026-09-07（二调）：服务端下发 `DOCTOR_APP_RESP` 增加 `patient_id[10]` → 存入 `APP_INFO.patient_id`；协议新增 `DOCTOR_SET_RECORD` + `SET_RECORD_REQ`{meet_id/doctor_id/patient_id/char diagnosis[200]/char treat_plan[200]}。`AppointDetailWidget` 暴露 `diagnosis()/treatPlan()`；`AppointWidget::openMeetDetail` 在弹窗 Accepted 后调 `sendRecord()`：按 getAppInfo 打包规范（`head.len=sizeof(body)`、`data.resize(HEAD+body)`、memcpy 拼包，诊断/处方 copyCStr 拷 UTF-8 带结束符）组 `HEAD(type=DOCTOR_SET_RECORD)+SET_RECORD_REQ`，经同一 `to_get_meet` 信号→SocketLink::send_data 发出（无回包，fire-and-forget，recv_data 无此类型分支属正常）。
- 2026-09-07（三调）：新增两套装饰性**工作统计**页并接入两窗导航“工作统计”(索引2) 与首页“工作统计”图标。普通医生端 `WorkStatWidget`（pans/workstatwidget，今日访问量/本月访问量/本月出勤天数/加班次数 4 指标卡）；管理员端 `ManagerStatWidget`（pans/mannger/managerstatwidget，文件内含 QPainter 自绘折线 `TrendChartWidget`）：内部 QStackedWidget 两视图——访问量趋势(日/周/月/年 分段按钮切换刻度→重随机生成) 与 医生工作量表(右上“医生工作量/查看趋势”按钮来回切；医生|月出勤天数|月访问量|月加班次数)。`CData` 加 `static bool is_check`（main.cpp 现置 true）：true=两页随机填充预留占位数据，false=留空/显示 “--”（等真实统计服务）。`SysWidget` 加 `to_workstat_page()` 信号。hos.pro 已注册两个新文件与目录 pans/mannger。
- 2026-09-09：`AppointDetailWidget`（接诊详情弹窗，医生填写病例处）改为**左右布局**：左侧保持原内容/外观不变，右侧新增舌苔图片位 `QLabel *m_tongueLabel`（成员，200×250 固定大小，圆角黑底占位 = 暂无图片，带“舌苔图片”标题与 tooltip，后续拿到图后对其 setPixmap 即可）；弹窗宽度 460→680。纯 UI 改动，不动协议/信号。二调：父页 setBack 的渐变 QSS 级联进弹窗导致内部 QLabel 透出蓝白渐变，弹窗 QSS 改为多规则、加 `AppointDetailWidget QLabel{background:transparent}` 整平（舌苔黑块自带 background 不受影响）。
- 2026-09-10：医生端接诊详情弹窗新增**一键导出 PDF**。`AppointDetailWidget` 底部左侧加“一键导出”开关按钮（点击变橙底“取消”，再点恢复“一键导出”；`exportPdfEnabled()` 对外暴露开关态），**开启**状态下点“完成”→ 二次确认 → `printAll()` → `exportPdf()` → accept()；导出失败只 warning、不阻断完成就诊（诊断处方照常发 `DOCTOR_SET_RECORD`），成功弹信息框报路径并存 `exportedFilePath()`。导出逻辑新文件 `Tool/recordpdf.{h,cpp}`（`RecordPdfData` + `RecordPdf::defaultDir/save/saveAs`）：**只用 QtGui**（QPdfWriter+QPainter+QTextDocument 手动分页，避开 QtPrintSupport/QPrinter，`hos.pro` 的 `QT +=` 不变），A4/96dpi/68px 页边距，存 `<系统文档>/诊疗记录/患者名_预约号_yyyyMMdd.pdf`（重名加 `_1/_2`），内容 = 标题 + 患者/预约号/医生/编号/时间 表 + 诊断 + 处方 + 舌苔图片（`m_tongueLabel->pixmap()` 空图则印“（暂无）”，非空经 `addResource` 嵌入）+ 导出时间页脚。hos.pro 已注册两个新文件。二调：PDF 版式**换成门诊病历表单模板、全文只用黑色**（原来蓝标题+浅蓝底表头+灰提示的装饰风取消），字体改宋体 SimSun，正文 10.5pt；内容改为“门 诊 病 历（诊疗记录）”居中标题 + 患者基本信息表（门诊号/就诊时间/患者姓名/患者编号/接诊医师/医师编号）+ 初步诊断/治疗意见（处方）/舌苔图像 三行分栏表 + 医师签名行（留空）+ 导出时间，表格统一 `border=1 bordercolor=#000000 cellpadding=7`、无 bgcolor，文字黑由 `PaintContext::palette` 设定。修了 `QStringLiteral(变量)` 与 `QFont base(QLatin1String(x))`（most vexing parse）两个编译坑。三调：明确**只允许黑与白**——“一键导出/取消”按钮由橙底改黑白反色，PDF 侧本来就只有一个颜色值 `#000000`、无 bgcolor，未再动。注意：弹窗其余控件（标题、返回、完成按钮）仍是全站浅蓝主题，未跟着改黑白。四调（已撤销）：曾按“不要边框”把 PDF 表格改 `border=0`、按钮改 `border:none`，用户随后要求撤销，已还原为三调状态（PDF 表 1px 黑线、按钮黑白反色带 1px 黑边）。
- 2026-09-10（六调）：普通医生端新增**“查看病例/病历”页** `pans/recordwidget.{h,cpp}`（代码式 UI），接到 `MainWindow` 左侧导航第 1 项（原来 `case 1: break;` 空接）与首页“查看病例”图标（`SysWidget::to_record_page()` 新信号 + `ui->toolButton`）；`mainwindow.cpp` 的 `init_stack_widget()` 里 `new RecordWidget` 并 add 到 stackedWidget（**add 顺序尾插**，`itemClicked` 改成 1→record_widget），`init_task_connect()` 里只留 TODO 注释、**不接任何 socket**。页内做全：姓名+日期区间查询、结果表、右侧可编辑表单、保存/撤销/换行脏检查。**数据通信按用户要求先不做**：不发包、不碰 protecol.h/socketlink/CData 缓存（`CData::is_check` 只管占位数据开关），上行信号 `to_query_record()/to_save_record()` 先留着不接。hos.pro 已注册新文件；ManngerWindow 未加此页（它按既有约定仍只保留 sys/workstat/doctor_order）。
- 2026-09-10（五调）：**导出内核整体换掉**：用户给出网页病历 HTML 模板（同 `widget.cpp` 那份）并要求“学习他”，定为**只换导出、弹窗 UI 不动**。`Tool/recordpdf.{h,cpp}` 从“QPdfWriter+QTextDocument 同步渲染”改写为 **QObject + 离屏 QWebEngineView 异步方案**：`exportRecord()/exportTo()` 载入 .cpp 内 `kRecordHtml`（成都中医药大学附属医院 门(急)诊病历，A4 210×297mm `.paper`、固定区 contenteditable=false、治疗意见/签名可编辑、只用黑白、`@media print` 去掉灰底），`loadFinished` 后 `runJavaScript` 走 JS 桥 `fillPatientInfo()`+`fillClinical()` 灌数据（患者名/登记号=meet_id/就诊时间/诊断/治疗意见=处方/医师签名=医生名/舌苔 dataURL），再 `printToPdf(A4, 0 边距)`，`pdfPrintingFinished` 发 `done(path,err)`；旧的 `save()/saveAs()` 同步 API、宋体 10.5pt 表格版式、`tableOpen()/rootDivOpen()/esc()/textBlock()/fitSize()` 等 QTextDocument 辅助全部删除。`AppointDetailWidget` 跟着改成异步：新增 `startExport()`/`m_exporter`/`m_exporting`，导出期间禁用按钮并忽略 `reject()`（Esc/关闭），`done` 回调里弹提示再 `accept()`。hus.pro 无需改动（`webenginewidgets` 本来就有）。
