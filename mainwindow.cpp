#include "mainwindow.h"
#include "ui_MainWindow.h"

#include <QLineEdit>
#include <QListView>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QtSvg/QSvgRenderer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtSvgWidgets/QGraphicsSvgItem>
#include <QPainter>
#include <QResizeEvent>
#include <QBrush>
#include <QColor>
#include <QIcon>

#include <QFile>
#include <QDir>
#include <QDebug>
#include <utility>

#include <QCoreApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include <QTimer>

#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>

// QXlsx
#include <xlsxdocument.h>
#include <xlsxformat.h>
#include <xlsxworksheet.h>
#include <xlsxworkbook.h>


// ====== listView 里为每条记录保存原始数据的角色 & 工具 =======
namespace {
enum {
    RoleCode13  = Qt::UserRole + 101,  // 原始 13 位 JAN
    RoleImei15  = Qt::UserRole + 102,  // 原始 15 位 IMEI
    RoleKind    = Qt::UserRole + 103,  // 入荷登録 / 仮登録 / 検索
    RoleBaseText= Qt::UserRole + 104   // 不含序号的“底稿文本”
};





// 容量显示：256->256GB, 512->512GB, 1024->1TB, 2048->2TB, ...
static QString capacityLabel(int gb) {
    if (gb >= 1024 && gb % 1024 == 0) return QStringLiteral("%1TB").arg(gb/1024);
    return QStringLiteral("%1GB").arg(gb);
}

// 颜色映射（你提供的清单）
static QString colorHexFor(const QString& model, const QString& color) {
    static const QHash<QString, QString> map = []{
        QHash<QString, QString> m;
        // iPhone 17
        m.insert(QStringLiteral("iPhone 17|ブラック"),        "#4a4d50");
        m.insert(QStringLiteral("iPhone 17|ホワイト"),         "#fafafa");
        m.insert(QStringLiteral("iPhone 17|ミストブルー"),      "#aabfde");
        m.insert(QStringLiteral("iPhone 17|ラベンダー"),        "#e7d9f2");
        m.insert(QStringLiteral("iPhone 17|セージ"),            "#bac69c");
        // iPhone 17 Pro
        m.insert(QStringLiteral("iPhone 17 Pro|シルバー"),          "#e7e7e7");
        m.insert(QStringLiteral("iPhone 17 Pro|コズミックオレンジ"), "#f6823d");
        m.insert(QStringLiteral("iPhone 17 Pro|ディープブルー"),      "#4b567f");
        // iPhone 17 Pro Max
        m.insert(QStringLiteral("iPhone 17 Pro Max|シルバー"),          "#e7e7e7");
        m.insert(QStringLiteral("iPhone 17 Pro Max|コズミックオレンジ"), "#f6823d");
        m.insert(QStringLiteral("iPhone 17 Pro Max|ディープブルー"),      "#4b567f");
        // iPhone Air
        m.insert(QStringLiteral("iPhone Air|スペースブラック"), "#171717");
        m.insert(QStringLiteral("iPhone Air|クラウドホワイト"), "#fcfcfc");
        m.insert(QStringLiteral("iPhone Air|ライトゴールド"),   "#faf3e6");
        m.insert(QStringLiteral("iPhone Air|スカイブルー"),     "#e5f2fa");
        return m;
    }();
    return map.value(model + "|" + color, QStringLiteral("#222222"));
}

// 目录行
struct CatRow {
    const char* part;
    const char* model;
    int         cap;
    const char* color;
    const char* date;
    const char* jan;
};

// 你的目录数据
static const CatRow kCatalogRows[] = {
    {"MG674J/A","iPhone 17",256,"ブラック","2025-09-19","4549995649154"},
    {"MG684J/A","iPhone 17",256,"ホワイト","2025-09-19","4549995649161"},
    {"MG694J/A","iPhone 17",256,"ミストブルー","2025-09-19","4549995649178"},
    {"MG6A4J/A","iPhone 17",256,"ラベンダー","2025-09-19","4549995649185"},
    {"MG6C4J/A","iPhone 17",256,"セージ","2025-09-19","4549995649192"},
    {"MG6D4J/A","iPhone 17",512,"ブラック","2025-09-19","4549995649208"},
    {"MG6E4J/A","iPhone 17",512,"ホワイト","2025-09-19","4549995649215"},
    {"MG6F4J/A","iPhone 17",512,"ミストブルー","2025-09-19","4549995649222"},
    {"MG6G4J/A","iPhone 17",512,"ラベンダー","2025-09-19","4549995649239"},
    {"MG6H4J/A","iPhone 17",512,"セージ","2025-09-19","4549995649246"},
    {"MG854J/A","iPhone 17 Pro",256,"シルバー","2025-09-19","4549995648294"},
    {"MG864J/A","iPhone 17 Pro",256,"コズミックオレンジ","2025-09-19","4549995648300"},
    {"MG874J/A","iPhone 17 Pro",256,"ディープブルー","2025-09-19","4549995648317"},
    {"MG894J/A","iPhone 17 Pro",512,"シルバー","2025-09-19","4549995648324"},
    {"MG8A4J/A","iPhone 17 Pro",512,"コズミックオレンジ","2025-09-19","4549995648331"},
    {"MG8C4J/A","iPhone 17 Pro",512,"ディープブルー","2025-09-19","4549995648348"},
    {"MG8D4J/A","iPhone 17 Pro",1024,"シルバー","2025-09-19","4549995648355"},
    {"MG8E4J/A","iPhone 17 Pro",1024,"コズミックオレンジ","2025-09-19","4549995648362"},
    {"MG8F4J/A","iPhone 17 Pro",1024,"ディープブルー","2025-09-19","4549995648379"},
    {"MFY84J/A","iPhone 17 Pro Max",256,"シルバー","2025-09-19","4549995649284"},
    {"MFY94J/A","iPhone 17 Pro Max",256,"コズミックオレンジ","2025-09-19","4549995649291"},
    {"MFYA4J/A","iPhone 17 Pro Max",256,"ディープブルー","2025-09-19","4549995649307"},
    {"MFYC4J/A","iPhone 17 Pro Max",512,"シルバー","2025-09-19","4549995649314"},
    {"MFYD4J/A","iPhone 17 Pro Max",512,"コズミックオレンジ","2025-09-19","4549995649321"},
    {"MFYE4J/A","iPhone 17 Pro Max",512,"ディープブルー","2025-09-19","4549995649338"},
    {"MFYF4J/A","iPhone 17 Pro Max",1024,"シルバー","2025-09-19","4549995649345"},
    {"MFYG4J/A","iPhone 17 Pro Max",1024,"コズミックオレンジ","2025-09-19","4549995649352"},
    {"MFYH4J/A","iPhone 17 Pro Max",1024,"ディープブルー","2025-09-19","4549995649369"},
    {"MFYJ4J/A","iPhone 17 Pro Max",2048,"シルバー","2025-09-19","4549995649376"},
    {"MFYK4J/A","iPhone 17 Pro Max",2048,"コズミックオレンジ","2025-09-19","4549995649383"},
    {"MFYL4J/A","iPhone 17 Pro Max",2048,"ディープブルー","2025-09-19","4549995649390"},
    {"MG274J/A","iPhone Air",256,"スペースブラック","2025-09-19","4549995647501"},
    {"MG284J/A","iPhone Air",256,"クラウドホワイト","2025-09-19","4549995647518"},
    {"MG294J/A","iPhone Air",256,"ライトゴールド","2025-09-19","4549995647525"},
    {"MG2A4J/A","iPhone Air",256,"スカイブルー","2025-09-19","4549995647532"},
    {"MG2C4J/A","iPhone Air",512,"スペースブラック","2025-09-19","4549995647549"},
    {"MG2D4J/A","iPhone Air",512,"クラウドホワイト","2025-09-19","4549995647556"},
    {"MG2E4J/A","iPhone Air",512,"ライトゴールド","2025-09-19","4549995647563"},
    {"MG2F4J/A","iPhone Air",512,"スカイブルー","2025-09-19","4549995647570"},
    {"MG2G4J/A","iPhone Air",1024,"スペースブラック","2025-09-19","4549995647587"},
    {"MG2H4J/A","iPhone Air",1024,"クラウドホワイト","2025-09-19","4549995647594"},
    {"MG2J4J/A","iPhone Air",1024,"ライトゴールド","2025-09-19","4549995647600"},
    {"MG2K4J/A","iPhone Air",1024,"スカイブルー","2025-09-19","4549995647617"},
    };

// 若 catalog 为空，则批量导入上面的 kCatalogRows
static bool seedCatalogIfEmpty(QSqlDatabase& db) {
    QSqlQuery qc(db);
    if (!qc.exec("SELECT COUNT(*) FROM catalog")) return false;
    if (!qc.next()) return false;
    if (qc.value(0).toInt() > 0) return true; // 已有数据，无需导入

    if (!db.transaction()) return false;
    QSqlQuery qi(db);
    qi.prepare(
        "INSERT OR IGNORE INTO catalog("
        " part_number, model_name, capacity_gb, color, release_date, jan, color_hex"
        ") VALUES(?,?,?,?,?,?,?)"
        );
    for (const auto& r : kCatalogRows) {
        qi.addBindValue(QString::fromUtf8(r.part));
        qi.addBindValue(QString::fromUtf8(r.model));
        qi.addBindValue(r.cap);
        qi.addBindValue(QString::fromUtf8(r.color));
        qi.addBindValue(QString::fromUtf8(r.date));
        qi.addBindValue(QString::fromUtf8(r.jan));
        qi.addBindValue(colorHexFor(QString::fromUtf8(r.model), QString::fromUtf8(r.color)));
        if (!qi.exec()) {
            qWarning() << "catalog insert failed:" << qi.lastError();
            db.rollback();
            return false;
        }
    }
    return db.commit();
}

// 由 JAN 取“型号 容量 颜色”，并返回颜色 hex（若有）
static QString displayNameForJan(const QSqlDatabase& db,  // <== 这里改成 const 引用
                                 const QString& jan,
                                 QString* colorHexOut = nullptr)
{
    QSqlQuery q(db);
    q.prepare("SELECT model_name, capacity_gb, color, color_hex "
              "FROM catalog WHERE jan = ? LIMIT 1");
    q.addBindValue(jan);
    if (!q.exec() || !q.next())
        return QString();

    const QString model = q.value(0).toString();
    const int     cap   = q.value(1).toInt();
    const QString color = q.value(2).toString();
    QString hex         = q.value(3).toString();
    if (hex.isEmpty())
        hex = colorHexFor(model, color);

    if (colorHexOut)
        *colorHexOut = hex;

    return QStringLiteral("%1 %2 %3").arg(model, capacityLabel(cap), color);
}

// —— 圆点图标 & 统一编号 —— //
static QPixmap makeColorDotPixmap(const QColor& c, int d = 18) {
    QPixmap px(d, d);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool light = c.lightness() > 180;
    QPen pen(light ? QColor(120,120,120) : c.darker(125), 1);
    p.setPen(pen);
    p.setBrush(c);
    p.drawEllipse(QRectF(1, 1, d-2, d-2));
    return px;
}
static void setItemColorDot(QStandardItem* it, const QString& hex, int diameter = 18) {
    if (!it) return;
    if (hex.isEmpty()) { it->setData(QVariant(), Qt::DecorationRole); return; }
    it->setData(QIcon(makeColorDotPixmap(QColor(hex), diameter)), Qt::DecorationRole);
}
static void renumberModel(QStandardItemModel* model) {
    if (!model) return;
    for (int i = 0; i < model->rowCount(); ++i) {
        if (auto* it = model->item(i)) {
            QString base = it->data(RoleBaseText).toString();
            if (base.isEmpty()) {
                static const QRegularExpression rx(QStringLiteral("^(\\d+)[\\.)]\\s+"));
                base = it->text(); base.remove(rx);
                it->setData(base, RoleBaseText);
            }
            const QString num = QStringLiteral("%1. ").arg(i + 1, 2, 10, QChar('0'));
            it->setText(num + base);
        }
    }
}
} // namespace

// ====================== 工具：fit + 额外放大 ======================
static void fitAndZoom(QGraphicsView *view,
                       double zoom = 1.3,
                       Qt::AspectRatioMode mode = Qt::KeepAspectRatio)
{
    if (!view || !view->scene() || view->scene()->items().isEmpty()) return;
    view->resetTransform();
    view->fitInView(view->scene()->itemsBoundingRect(), mode);
    if (zoom > 1.0) view->scale(zoom, zoom);
}

// —— 特殊码 —— //
static const QString kCodeToSearch   = QStringLiteral("2222222222222");   // 跳到 検索(lineEdit_6)
static const QString kCodeToRegister = QStringLiteral("5555555555555");   // 入荷登録：搜索/仮登録 -> lineEdit
static const QString kCodeToTemp     = QStringLiteral("3333333333333");   // 跳到 仮登録(lineEdit_4)
static const QString kCodeResetCount = QStringLiteral("1111111111111");   // 计数器2清零
static const QString kCodeFlushAll   = QStringLiteral("4444444444444");   // 仮登録列表批量落库

// —— 工具：仅包含某种来源（优先读 RoleKind，兼容旧文本）—— //
static bool modelOnlyHasPrefix(const QStandardItemModel* model, const QString& kindTag)
{
    if (!model || model->rowCount() == 0) return false;
    const QString bracket = QStringLiteral("[%1]").arg(kindTag);
    for (int r = 0; r < model->rowCount(); ++r) {
        const QStandardItem* it = model->item(r);
        if (!it) continue;
        const QString roleKind = it->data(RoleKind).toString();
        if (!roleKind.isEmpty()) {
            if (roleKind != kindTag) return false;
        } else {
            if (!it->text().contains(bracket)) return false; // 兼容老行
        }
    }
    return true;
}

// —— 工具：精确长度校验，不符合则清空+红底+状态栏提示 —— //
static bool ensureExactLenAndMark(QLineEdit* w, int expected, QStatusBar* bar, const QString& name)
{
    const QString t = w->text().trimmed();
    if (t.size() == expected) return true;
    w->clear();
    w->setStyleSheet("QLineEdit { background-color: #ffcccc; }"); // 红底
    if (bar) bar->showMessage(QStringLiteral("%1 需要 %2 位数字，已清空。").arg(name).arg(expected), 2500);
    return false;
}

// ====================== 焦点高亮 ======================
FocusHighlighter::FocusHighlighter(const QList<QLineEdit*>& targets, QObject* parent)
    : QObject(parent), m_targets(targets)
{
    for (QLineEdit* w : std::as_const(m_targets))
        m_defaultStyles[w] = w->styleSheet();
}
void FocusHighlighter::clearAll()
{
    for (QLineEdit* w : std::as_const(m_targets))
        w->setStyleSheet(m_defaultStyles.value(w));
}
bool FocusHighlighter::eventFilter(QObject* obj, QEvent* event)
{
    auto* w = qobject_cast<QLineEdit*>(obj);
    if (!w || !m_targets.contains(w)) return QObject::eventFilter(obj, event);
    switch (event->type())
    {
    case QEvent::FocusIn:
        clearAll();
        w->setStyleSheet("QLineEdit { background-color: #c9f7c3; }"); // 淡绿
        break;
    case QEvent::FocusOut:
        w->setStyleSheet(m_defaultStyles.value(w));
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

// ====================== 限制输入域 ======================
ScannerOnlyGuard::ScannerOnlyGuard(const QList<QLineEdit*>& allowed, QObject* parent)
    : QObject(parent)
{
    for (auto* w : allowed)
        m_allowed.insert(w);
}
bool ScannerOnlyGuard::isDigitOrBackspace(int key) const
{
    return (key >= Qt::Key_0 && key <= Qt::Key_9) || key == Qt::Key_Backspace;
}
bool ScannerOnlyGuard::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj);
    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(obj, event);
    auto* kev = static_cast<QKeyEvent*>(event);
    if (kev->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return QObject::eventFilter(obj, event);
    QWidget* fw = QApplication::focusWidget();
    bool inAllowed = m_allowed.contains(fw);
    if (!inAllowed && isDigitOrBackspace(kev->key()))
        return true; // 吞掉
    return QObject::eventFilter(obj, event);
}

// ====================== MainWindow ======================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new QStandardItemModel(this))
    , m_modelSession(new QStandardItemModel(this))
    , m_highlighter(nullptr)
    , m_guard(nullptr)
{
    ui->setupUi(this);

    // SVG：加载到 8 个 graphicsView_*
    setSvgToView(ui->graphicsView_6,  QStringLiteral(":/Reset.svg"));
    setSvgToView(ui->graphicsView_7,  QStringLiteral(":/Search.svg"));
    setSvgToView(ui->graphicsView_8,  QStringLiteral(":/Stock_Registration.svg"));
    setSvgToView(ui->graphicsView_9,  QStringLiteral(":/Temporary_Registration.svg"));
    setSvgToView(ui->graphicsView_10, QStringLiteral(":/Temporary_Registration_Write.svg"));
    setSvgToView(ui->graphicsView_11, QStringLiteral(":/View_Summary.svg"));
    setSvgToView(ui->graphicsView_12, QStringLiteral(":/Excel_Output.svg"));
    setSvgToView(ui->graphicsView_13, QStringLiteral(":/Excel_Display.svg"));

    // listView 绑定模型（左/右）+ 设置图标尺寸（大圆点）
    ui->listView->setModel(m_model);
    ui->listView_2->setModel(m_modelSession);
    ui->listView->setIconSize(QSize(18, 18));
    ui->listView_2->setIconSize(QSize(18, 18));

    // 仅这 6 个 QLineEdit 允许扫码输入
    m_scannerEdits = {
        ui->lineEdit,   ui->lineEdit_2,
        ui->lineEdit_6, ui->lineEdit_5,
        ui->lineEdit_4, ui->lineEdit_3
    };

    initValidators();

    // 焦点高亮
    m_highlighter = new FocusHighlighter(m_scannerEdits, this);
    for (QLineEdit* w : std::as_const(m_scannerEdits))
        w->installEventFilter(m_highlighter);

    // 应用级扫码限制
    m_guard = new ScannerOnlyGuard(m_scannerEdits, this);
    qApp->installEventFilter(m_guard);

    initConnections();

    // Tab 顺序（与扫码 Enter 不冲突）
    QWidget::setTabOrder(ui->lineEdit,   ui->lineEdit_2);
    QWidget::setTabOrder(ui->lineEdit_6, ui->lineEdit_5);
    QWidget::setTabOrder(ui->lineEdit_4, ui->lineEdit_3);

    // —— 打开数据库 & 会话持久化 —— //
    if (!initDatabase() && ui->statusbar)
        ui->statusbar->showMessage(QStringLiteral("数据库初始化失败，请检查权限/路径。"), 4000);

    // 会话选择（继续上次 / 新建）
    chooseOrCreateSessionOnStartup();

    // 记住状态栏默认样式
    m_statusDefaultStyle = ui->statusbar ? ui->statusbar->styleSheet() : QString();

    // 同步计数与右侧会话列表
    updateLcdFromDb();
    refreshSessionRecordsView();

    // 默认焦点
    ui->lineEdit->setFocus();

    // 让 Excel 出力/表示 图标可点击
    ui->graphicsView_12->setCursor(Qt::PointingHandCursor);
    ui->graphicsView_13->setCursor(Qt::PointingHandCursor);
    ui->graphicsView_12->setToolTip(QStringLiteral("Excel出力"));
    ui->graphicsView_13->setToolTip(QStringLiteral("Excel表示"));
    ui->graphicsView_12->installEventFilter(this);
    ui->graphicsView_13->installEventFilter(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* e)
{
    if ((obj == ui->graphicsView_12 || obj == ui->graphicsView_13)
        && e->type() == QEvent::MouseButtonRelease) {
        if (obj == ui->graphicsView_12) {
            exportToExcel();
        } else {
            openLastExport();
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, e);
}

QVector<ExportRow> MainWindow::gatherCurrentSessionRows() const
{
    QVector<ExportRow> rows;
    QSqlQuery q(m_db);
    q.prepare("SELECT code13, imei15, created_at FROM inbound "
              "WHERE session_id=? AND kind='入荷登録' ORDER BY id ASC");
    q.addBindValue(m_sessionId);
    if (!q.exec()) {
        qWarning() << "gatherCurrentSessionRows failed:" << q.lastError();
        return rows;
    }
    int seq = 1;
    while (q.next()) {
        const QString jan  = q.value(0).toString();
        const QString imei = q.value(1).toString();
        QString hex;
        const QString disp = displayNameForJan(m_db, jan, &hex);
        ExportRow r;
        r.seq         = seq++;
        r.jan         = jan;
        r.productName = disp.isEmpty() ? jan : disp;
        r.imei        = imei;
        r.qty         = 1;
        r.unitPrice   = 0.0; // 目前未知，留 0（可后续从 DB/配置读取）
        rows.push_back(r);
    }
    return rows;
}


bool MainWindow::writeExportedItemsSheet(QXlsx::Document& xlsx,
                                         const QVector<ExportRow>& rows,
                                         double* totalAmountOut)
{
    using namespace QXlsx;

    // ---- 先按 JAN 汇总：Product 用目录名（机型 + 容量(GB/TB) + 颜色），Qty 为台数合计 ----
    struct Agg { QString product; int qty = 0; };
    QMap<QString, Agg> agg;
    for (const auto& e : rows) {
        auto &a = agg[e.jan];
        if (a.product.isEmpty()) {
            QString hex;
            const QString disp = displayNameForJan(m_db, e.jan, &hex);
            a.product = disp.isEmpty() ? e.jan : disp;
        }
        a.qty += e.qty; // 你的 ExportRow.qty 目前恒为 1
    }

    xlsx.addSheet("Exported_Items");
    xlsx.selectSheet("Exported_Items");

    // 样式
    Format bold;  bold.setFontBold(true);
    Format th;    th.setFontBold(true);
    th.setHorizontalAlignment(Format::AlignHCenter);
    Format right; right.setHorizontalAlignment(Format::AlignRight);
    Format center;center.setHorizontalAlignment(Format::AlignHCenter);

    // 顶部 合計 / 合計金額
    int totalQty = 0;
    for (auto it = agg.cbegin(); it != agg.cend(); ++it) totalQty += it.value().qty;

    int r = 1;
    xlsx.write(r, 1, QStringLiteral("合計"), bold);
    xlsx.write(r, 2, totalQty, right);
    xlsx.write(r, 4, QStringLiteral("合計金額"), bold);
    xlsx.write(r, 5, 0, right);   // 暂无单价配置 → 金額先记 0
    r += 2;

    // 上半部分表头（按 JAN 汇总）
    xlsx.write(r,1, "JAN",          th);
    xlsx.write(r,2, "Product",      th);
    xlsx.write(r,3, "Qty",          th);
    xlsx.write(r,4, "Unit Price",   th);
    xlsx.write(r,5, "Total Amount", th);
    ++r;

    // 上半部分数据（每个 JAN 仅 1 行）
    for (auto it = agg.cbegin(); it != agg.cend(); ++it) {
        xlsx.write(r,1, it.key());
        xlsx.write(r,2, it.value().product);
        xlsx.write(r,3, it.value().qty, center);
        xlsx.write(r,4, 0, right);     // 单价=0
        xlsx.write(r,5, 0, right);     // 金額=0
        ++r;
    }

    // 空两行后，输出明细（逐台 IMEI）
    r += 2;
    xlsx.write(r,1, QStringLiteral("番号"),     th);
    xlsx.write(r,2, QStringLiteral("JANコード"), th);
    xlsx.write(r,3, QStringLiteral("型番"),     th);
    xlsx.write(r,4, QStringLiteral("数量"),     th);
    xlsx.write(r,5, QStringLiteral("IMEI番号"), th);
    ++r;

    for (const auto& e : rows) {
        xlsx.write(r,1, e.seq, center);
        xlsx.write(r,2, e.jan);
        xlsx.write(r,3, e.productName);
        xlsx.write(r,4, e.qty, center); // 逐条=1
        xlsx.write(r,5, e.imei);
        ++r;
    }

    // 日期/署名
    ++r;
    xlsx.write(r,2, QStringLiteral("日付：%1")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")));
    xlsx.write(r,5, QStringLiteral("ご署名："));

    // 列宽
    xlsx.setColumnWidth(1, 1, 18);
    xlsx.setColumnWidth(2, 2, 46);
    xlsx.setColumnWidth(3, 3, 8);
    xlsx.setColumnWidth(4, 5, 14);

    if (totalAmountOut) *totalAmountOut = 0;
    return true;
}


bool MainWindow::writeWs3Sheet(QXlsx::Document& xlsx,
                               const QVector<ExportRow>& rows)
{
    using namespace QXlsx;

    xlsx.addSheet("ws3");
    xlsx.selectSheet("ws3");

    Format th; th.setFontBold(true);

    // 不要“社内番号”
    const QStringList headers = {
        QStringLiteral("会員番号"),
        QStringLiteral("仕入れ先"),
        QStringLiteral("店頭/郵送/自社ネット"),
        QStringLiteral("注文番号"),
        QStringLiteral("注文日期"),
        QStringLiteral("到着日"),
        QStringLiteral("JAN"),
        QStringLiteral("商品名"),
        QStringLiteral("IMEI"),
        QStringLiteral("数量"),
        QStringLiteral("単価"),
        QStringLiteral("金額"),
        QStringLiteral("送料")
    };

    int r = 1;
    for (int c = 0; c < headers.size(); ++c)
        xlsx.write(r, c + 1, headers[c], th);
    ++r;

    // ---- 按 JAN 汇总：每个 JAN 只写一行，IMEI 留空 ----
    struct Agg { QString product; int qty = 0; };
    QMap<QString, Agg> agg;
    for (const auto& e : rows) {
        auto &a = agg[e.jan];
        if (a.product.isEmpty()) a.product = e.productName; // 已是“机型 容量 颜色”
        a.qty += e.qty;                                     // 逐条=1
    }

    for (auto it = agg.cbegin(); it != agg.cend(); ++it) {
        int c = 1;
        // 会員番号..到着日（6列）留空（后续可从 UI/设置填）
        for (int k = 0; k < 6; ++k) xlsx.write(r, c++, "");
        xlsx.write(r, c++, it.key());            // JAN
        xlsx.write(r, c++, it.value().product);  // 商品名
        xlsx.write(r, c++, "");                  // IMEI（聚合留空）
        xlsx.write(r, c++, it.value().qty);      // 数量
        xlsx.write(r, c++, 0);                   // 単価 先 0
        xlsx.write(r, c++, 0);                   // 金額 先 0
        xlsx.write(r, c++, 0);                   // 送料 先 0
        ++r;
    }

    // 列宽
    xlsx.setColumnWidth(1, 6, 16);
    xlsx.setColumnWidth(7, 7, 18);  // JAN
    xlsx.setColumnWidth(8, 8, 46);  // 商品名
    xlsx.setColumnWidth(9,13, 14);  // IMEI/数量/単価/金額/送料
    return true;
}


void MainWindow::exportToExcel()
{
    const auto rows = gatherCurrentSessionRows();
    if (rows.isEmpty()) {
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("当前会话无“入荷登録”记录，未生成。"), 2500);
        return;
    }

    // 保存到桌面
    const QString desk = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QDir dir(desk); if (!dir.exists()) dir.mkpath(".");
    const QString filename = QStringLiteral("入荷出力_%1.xlsx")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = dir.filePath(filename);

    QXlsx::Document xlsx;
    double totalAmount = 0.0;
    writeExportedItemsSheet(xlsx, rows, &totalAmount);
    writeWs3Sheet(xlsx, rows);
    xlsx.selectSheet("Exported_Items"); // 默认选中

    if (!xlsx.saveAs(path)) {
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("Excel出力失败。"), 2500);
        return;
    }

    m_lastExportPath = path;
    showStatusOk(QStringLiteral("Excel出力完了：%1").arg(path));
}

void MainWindow::openLastExport()
{
    if (m_lastExportPath.isEmpty()) {
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("まだファイルがありません。先にExcel出力してください。"), 2500);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastExportPath));
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    fitAndZoom(ui->graphicsView_6);
    fitAndZoom(ui->graphicsView_7);
    fitAndZoom(ui->graphicsView_8);
    fitAndZoom(ui->graphicsView_9);
    fitAndZoom(ui->graphicsView_10);
    fitAndZoom(ui->graphicsView_11);
    fitAndZoom(ui->graphicsView_12);
    fitAndZoom(ui->graphicsView_13);
}

void MainWindow::initValidators()
{
    // 输入时只允许“最多 N 位”，Enter 时再做“必须 == N 位”的精确校验
    const QRegularExpression rx13(QStringLiteral("^\\d{0,13}$"));
    const QRegularExpression rx15(QStringLiteral("^\\d{0,15}$"));
    ui->lineEdit->setValidator   (new QRegularExpressionValidator(rx13, ui->lineEdit));
    ui->lineEdit_6->setValidator (new QRegularExpressionValidator(rx13, ui->lineEdit_6));
    ui->lineEdit_4->setValidator (new QRegularExpressionValidator(rx13, ui->lineEdit_4));
    ui->lineEdit_2->setValidator (new QRegularExpressionValidator(rx15, ui->lineEdit_2));
    ui->lineEdit_3->setValidator (new QRegularExpressionValidator(rx15, ui->lineEdit_3));
    ui->lineEdit_5->setValidator (new QRegularExpressionValidator(rx15, ui->lineEdit_5));
}

void MainWindow::initConnections()
{
    // 入荷登録
    connect(ui->lineEdit,   &QLineEdit::returnPressed, this, &MainWindow::onReg1Enter);
    connect(ui->lineEdit_2, &QLineEdit::returnPressed, this, &MainWindow::onReg2Enter);

    // 検索
    connect(ui->lineEdit_6, &QLineEdit::returnPressed, this, &MainWindow::onSearch1Enter);
    connect(ui->lineEdit_5, &QLineEdit::returnPressed, this, &MainWindow::onSearch2Enter);

    // 仮登録
    connect(ui->lineEdit_4, &QLineEdit::returnPressed, this, &MainWindow::onTemp1Enter);
    connect(ui->lineEdit_3, &QLineEdit::returnPressed, this, &MainWindow::onTemp2Enter);

    // “リセット”按钮
    if (ui->pushButton)
        connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);
}

// ====================== SVG 显示 ======================
void MainWindow::setSvgToView(QGraphicsView* view,
                              const QString& qrcPath,
                              const QString& elementId,
                              Qt::AspectRatioMode mode)
{
    if (!view) return;
    QGraphicsScene* scene = view->scene();
    if (!scene) {
        scene = new QGraphicsScene(view);
        view->setScene(scene);
    } else {
        scene->clear();
    }
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setRenderHint(QPainter::TextAntialiasing, true);
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QGraphicsSvgItem* item = nullptr;
    if (elementId.isEmpty()) {
        item = new QGraphicsSvgItem(qrcPath);
        if (!item->renderer() || !item->renderer()->isValid()) { delete item; return; }
    } else {
        auto* renderer = new QSvgRenderer(qrcPath, view);
        if (!renderer->isValid()) { delete renderer; return; }
        item = new QGraphicsSvgItem;
        item->setSharedRenderer(renderer);
        item->setElementId(elementId);
    }
    item->setFlag(QGraphicsItem::ItemClipsToShape, true);
    scene->addItem(item);
    scene->setSceneRect(item->boundingRect());
    fitAndZoom(view, 1.3, mode);
}

// ====================== 列表与状态栏 ======================
QString MainWindow::formatRecord(const QString& prefix, const QStringList& parts) const
{
    QStringList filtered;
    for (const auto& p : parts)
        if (!p.trimmed().isEmpty()) filtered << p.trimmed();
    const QString content = filtered.join(QStringLiteral(" / "));
    return content.isEmpty() ? QStringLiteral("[%1]").arg(prefix)
                             : QStringLiteral("[%1] %2").arg(prefix, content);
}
QString MainWindow::formatRecord(const QString& prefix, const QString& parts) const
{
    return parts.trimmed().isEmpty()
    ? QStringLiteral("[%1]").arg(prefix)
    : QStringLiteral("[%1] %2").arg(prefix, parts.trimmed());
}

void MainWindow::appendListDirect(const QString& text)
{
    m_model->appendRow(new QStandardItem(text));
    // 不修改 m_source
}

void MainWindow::addToListWithSource(const QString& text, ListSource src, const QColor& fgColor)
{
    auto makeItem = [&](const QString& s)->QStandardItem* {
        auto* it = new QStandardItem(s);
        if (fgColor.isValid())
            it->setForeground(QBrush(fgColor)); // 仅用于“重复→红色”这样的场景
        it->setData(s, RoleBaseText);           // 保存底稿，方便统一编号
        return it;
    };

    if (m_source == src) {
        m_model->appendRow(makeItem(text));
    } else {
        m_model->clear();
        m_model->appendRow(makeItem(text));
        m_source = src;
    }
    renumberModel(m_model); // 来源切换或追加后统一编号
}

void MainWindow::showStatusOk(const QString& text)
{
    if (!ui->statusbar) return;
    if (m_statusDefaultStyle.isEmpty())
        m_statusDefaultStyle = ui->statusbar->styleSheet();

    ui->statusbar->setStyleSheet(QStringLiteral("QStatusBar{ color:#0a6d2a; }")); // 深绿色
    ui->statusbar->showMessage(text, 2500);

    QTimer::singleShot(2500, this, [this]{
        ui->statusbar->setStyleSheet(m_statusDefaultStyle);
    });
}

// ====================== 数据库 & 会话 ======================
bool MainWindow::initDatabase()
{
    if (QSqlDatabase::contains("main"))
        m_db = QSqlDatabase::database("main");
    else
        m_db = QSqlDatabase::addDatabase("QSQLITE", "main");

    const QString dbPath = QDir(QCoreApplication::applicationDirPath())
                               .filePath(QStringLiteral("iphone_stock.sqlite"));
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "DB open failed:" << m_db.lastError().text();
        return false;
    }
    if (!ensureSchema()) return false;

    // 首次启动导入目录数据
    seedCatalogIfEmpty(m_db);
    return true;
}

bool MainWindow::ensureSchema()
{
    QSqlQuery q(m_db);

    q.exec("PRAGMA journal_mode=WAL;");
    q.exec("PRAGMA synchronous=NORMAL;");

    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS inbound ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  session_id TEXT NOT NULL,"
            "  kind TEXT NOT NULL,"
            "  code13 TEXT NOT NULL,"
            "  imei15 TEXT NOT NULL,"
            "  created_at TEXT DEFAULT (datetime('now','localtime'))"
            ");"
            )) { qWarning() << q.lastError(); return false; }

    if (!q.exec(
            "CREATE UNIQUE INDEX IF NOT EXISTS uq_inbound_sess_kind_imei "
            "ON inbound(session_id, kind, imei15);"
            )) { qWarning() << q.lastError(); return false; }

    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS entry_log ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  session_id TEXT NOT NULL,"
            "  type TEXT NOT NULL,"
            "  left_code TEXT,"
            "  right_code TEXT,"
            "  created_at TEXT DEFAULT (datetime('now','localtime'))"
            ");"
            )) { qWarning() << q.lastError(); return false; }

    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS catalog ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  part_number TEXT NOT NULL,"
            "  model_name  TEXT NOT NULL,"
            "  capacity_gb INTEGER NOT NULL,"
            "  color       TEXT NOT NULL,"
            "  release_date TEXT,"
            "  jan         TEXT NOT NULL UNIQUE,"
            "  color_hex   TEXT"
            ");"
            )) { qWarning() << q.lastError(); return false; }

    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS uq_catalog_part ON catalog(part_number);");
    q.exec("CREATE UNIQUE INDEX IF NOT EXISTS uq_catalog_jan  ON catalog(jan);");

    return true;
}

bool MainWindow::insertInboundRow(const QString& kind,
                                  const QString& code13,
                                  const QString& imei15,
                                  QString* errText)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO inbound(session_id, kind, code13, imei15) VALUES(?,?,?,?)");
    q.addBindValue(m_sessionId);
    q.addBindValue(kind);
    q.addBindValue(code13);
    q.addBindValue(imei15);
    const bool ok = q.exec();
    if (!ok && errText) *errText = q.lastError().text();
    return ok;
}

bool MainWindow::insertEntryLogRow(const QString& type,
                                   const QString& leftCode,
                                   const QString& rightCode)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO entry_log(session_id, type, left_code, right_code) VALUES(?,?,?,?)");
    q.addBindValue(m_sessionId);
    q.addBindValue(type);
    q.addBindValue(leftCode);
    q.addBindValue(rightCode);
    return q.exec();
}

bool MainWindow::existsInboundImeiInCurrentSession(const QString& imei15) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM inbound WHERE session_id=? AND kind='入荷登録' AND imei15=? LIMIT 1");
    q.addBindValue(m_sessionId);
    q.addBindValue(imei15);
    if (!q.exec()) return false;
    return q.next();
}

int MainWindow::countInboundRowsForSessionKind(const QString& kind) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT COUNT(*) FROM inbound WHERE session_id=? AND kind=?");
    q.addBindValue(m_sessionId);
    q.addBindValue(kind);
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toInt();
}

void MainWindow::updateLcdFromDb()
{
    const int total = countInboundRowsForSessionKind(QStringLiteral("入荷登録"));
    ui->lcdNumber->display(total);
}

bool MainWindow::hasTempInListView() const
{
    for (int r = 0; r < m_model->rowCount(); ++r) {
        const auto *it = m_model->item(r);
        if (!it) continue;
        const QString roleKind = it->data(RoleKind).toString();
        if (roleKind == QStringLiteral("仮登録")) return true;
        if (roleKind.isEmpty() && it->text().contains(QStringLiteral("[仮登録]"))) return true; // 兼容
    }
    return false;
}

bool MainWindow::flushAllListItemsToDb()
{
    bool wroteInbound = false;

    for (int r = 0; r < m_model->rowCount(); ++r) {
        QStandardItem* it = m_model->item(r);
        if (!it) continue;

        QString origKind = it->data(RoleKind).toString();
        QString a        = it->data(RoleCode13).toString();
        QString b        = it->data(RoleImei15).toString();

        // 必须从隐藏角色中拿到有效数据，否则跳过（避免“10个已完成”等文本行）
        if (origKind.isEmpty() || a.isEmpty() || b.isEmpty()) continue;
        if (origKind != QStringLiteral("入荷登録") && origKind != QStringLiteral("仮登録"))
            continue;

        // 统一：按 入荷登録 落库
        const QString kindNorm = QStringLiteral("入荷登録");

        // 去重（同会话/同 kind）
        const bool isDup = existsInboundImeiInCurrentSession(b);

        // 记录日志
        insertEntryLogRow(kindNorm, a, b);

        if (isDup) {
            // 原仮登録的重复项→红字提示
            if (origKind == QStringLiteral("仮登録"))
                it->setForeground(QBrush(QColor("#d32f2f")));
            continue;
        }

        QString err;
        if (insertInboundRow(kindNorm, a, b, &err)) {
            wroteInbound = true;
        } else if (!err.isEmpty()) {
            qWarning() << "flush insert failed:" << err;
        }
    }

    return wroteInbound;
}

// —— 会话持久化 —— //
QString MainWindow::generateSessionId() const
{
    return QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz");
}
QString MainWindow::readLastSessionIdQSettings() const
{
    QSettings s("Syu", "iPhoneStockManagementSystem");
    return s.value("last_session_id").toString();
}
void MainWindow::writeLastSessionIdQSettings(const QString& sid)
{
    QSettings s("Syu", "iPhoneStockManagementSystem");
    s.setValue("last_session_id", sid);
}

void MainWindow::chooseOrCreateSessionOnStartup()
{
    const QString lastSid = readLastSessionIdQSettings();
    if (!lastSid.isEmpty()) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(QStringLiteral("会话选择"));
        box.setText(QStringLiteral("检测到上次操作（会话ID：%1）。\n是否继续上次操作？").arg(lastSid));
        QPushButton* btnContinue = box.addButton(QStringLiteral("继续上次操作"), QMessageBox::AcceptRole);
        QPushButton* btnNew      = box.addButton(QStringLiteral("开启新操作"), QMessageBox::DestructiveRole);
        box.setDefaultButton(btnContinue);
        box.exec();

        if (box.clickedButton() == btnContinue) {
            m_sessionId = lastSid;
            if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("已继续上次操作。"), 1500);
            return;
        }
    }
    m_sessionId = generateSessionId();
    writeLastSessionIdQSettings(m_sessionId);
    if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("已开启新操作。"), 1500);
}

// —— 会话记录面板（右侧） —— //
void MainWindow::refreshSessionRecordsView()
{
    if (!m_modelSession) return;
    m_modelSession->clear();

    QSqlQuery q(m_db);
    q.prepare("SELECT created_at, kind, code13, imei15 "
              "FROM inbound WHERE session_id = ? ORDER BY id ASC");
    q.addBindValue(m_sessionId);

    if (!q.exec()) {
        qWarning() << "refreshSessionRecordsView failed:" << q.lastError();
        return;
    }

    while (q.next()) {
        const QString ts   = q.value(0).toString();
        const QString kind = q.value(1).toString();
        const QString c13  = q.value(2).toString();
        const QString i15  = q.value(3).toString();

        QString hex;
        const QString disp = displayNameForJan(m_db, c13, &hex);
        const QString left = disp.isEmpty() ? c13 : disp;

        const QString baseText = QStringLiteral("%1  [%2] %3 / %4")
                                     .arg(ts, kind, left, i15);

        auto* it = new QStandardItem(baseText);
        it->setData(baseText, RoleBaseText);
        setItemColorDot(it, hex);   // 仅圆点上色，文字保持黑色
        m_modelSession->appendRow(it);
    }

    renumberModel(m_modelSession);
}

// ====================== 槽：入荷登録 ======================
void MainWindow::onReg1Enter()
{
    const QString v = ui->lineEdit->text().trimmed();

    if (v == kCodeFlushAll) {
        ui->lineEdit->clear();
        if (hasTempInListView()) {
            if (flushAllListItemsToDb()) {
                updateLcdFromDb();
                refreshSessionRecordsView();
                showStatusOk(QStringLiteral("記録完了"));
                m_model->clear();
                m_source = ListSource::None;
                m_lcd2Counter = 0;
                ui->lcdNumber_2->display(0);
            }
        }
        return;
    }
    if (v == kCodeResetCount) {
        m_lcd2Counter = 0;
        ui->lcdNumber_2->display(m_lcd2Counter);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("计数器2 已清零。"), 2000);
        ui->lineEdit->clear();
        return;
    }
    if (v == kCodeToSearch) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 搜索。"), 1500);
        return;
    }
    if (v == kCodeToTemp) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 仮登録。"), 1500);
        return;
    }

    if (!ensureExactLenAndMark(ui->lineEdit, 13, ui->statusbar, QStringLiteral("入荷登録(前码)")))
        return;

    if (ui->label) { ui->label->setText(QStringLiteral("ログ")); ui->label->setStyleSheet(QString()); }
    ui->lineEdit_2->setFocus(); ui->lineEdit_2->selectAll();
}

void MainWindow::onReg2Enter()
{
    const QString a = ui->lineEdit->text().trimmed();
    const QString b = ui->lineEdit_2->text().trimmed();

    if (b == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("计数器2 已清零。"), 1500);
        return;
    }
    if (b == kCodeFlushAll) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb(); refreshSessionRecordsView(); showStatusOk(QStringLiteral("記録完了"));
            m_model->clear(); m_source = ListSource::None; m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
            if (ui->label_13) ui->label_13->setText(QStringLiteral("検索結果"));
            ui->lineEdit->setFocus();
        }
        return;
    }
    if (b == kCodeToSearch) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 搜索。"), 1500);
        return;
    }
    if (b == kCodeToTemp) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 仮登録。"), 1500);
        return;
    }

    if (!ensureExactLenAndMark(ui->lineEdit_2, 15, ui->statusbar, QStringLiteral("入荷登録(后码)")))
        return;

    if (a.size() != 13) {
        ui->lineEdit->clear();
        ui->lineEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("请先在 入荷登録(前码) 输入 13 位。"), 2500);
        return;
    }

    if (existsInboundImeiInCurrentSession(b)) {
        ui->label->setText(QStringLiteral("IMEI重複"));
        ui->label->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        ui->lineEdit_2->clear();
        ui->lineEdit_2->setFocus(); ui->lineEdit_2->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録：IMEI 重複，未记录。"), 2000);
        return;
    }

    QString err;
    const bool okInbound = insertInboundRow(QStringLiteral("入荷登録"), a, b, &err);
    const bool okLog     = insertEntryLogRow(QStringLiteral("入荷登録"), a, b);
    if (!okInbound) {
        qWarning() << "insert inbound failed:" << err;
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録：写库失败。"), 2000);
    }

    // —— 列表：机型名 + 彩点 + 序号（文字黑色）—— //
    QString hex;
    const QString disp = displayNameForJan(m_db, a, &hex);
    const QString left = disp.isEmpty() ? a : disp;
    const QString baseText = formatRecord(QStringLiteral("入荷登録"), {left, b});

    auto* it = new QStandardItem(baseText);
    it->setData(baseText, RoleBaseText);
    it->setData(a, RoleCode13);
    it->setData(b, RoleImei15);
    it->setData(QStringLiteral("入荷登録"), RoleKind);
    setItemColorDot(it, hex); // 只设置圆点

    m_model->appendRow(it);
    renumberModel(m_model);

    // 提示 + label 显示机型名（按机型色）
    showStatusOk(QStringLiteral("記録完了"));
    if (ui->label) {
        if (!disp.isEmpty()) {
            ui->label->setText(disp);
            ui->label->setStyleSheet(hex.isEmpty() ? QString()
                                                   : QStringLiteral("QLabel{ color:%1; }").arg(hex));
        } else {
            ui->label->setText(QStringLiteral("ログ"));
            ui->label->setStyleSheet(QString());
        }
    }

    // 计数器：lcdNumber_2 +1 且 /10 清零；lcdNumber 从 DB 刷新
    m_lcd2Counter = (m_lcd2Counter + 1) % 10;
    ui->lcdNumber_2->display(m_lcd2Counter);
    updateLcdFromDb();
    refreshSessionRecordsView();

    // 满 10 条入荷登録则清空并显示“10个已完成”
    int regCount = 0;
    for (int r = 0; r < m_model->rowCount(); ++r) {
        if (auto *line = m_model->item(r)) {
            if (line->data(RoleKind).toString() == QStringLiteral("入荷登録")) ++regCount;
        }
    }
    if (regCount == 10) {
        m_model->clear();
        auto* msg = new QStandardItem(QStringLiteral("10个已完成"));
        msg->setData(QStringLiteral("10个已完成"), RoleBaseText);
        m_model->appendRow(msg);
        renumberModel(m_model);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録：10个已完成，列表已重置。"), 2000);
    }

    ui->lineEdit->clear(); ui->lineEdit_2->clear(); ui->lineEdit->setFocus();
}

// ====================== 槽：仮登録 ======================
void MainWindow::onTemp1Enter()
{
    const QString v = ui->lineEdit_4->text().trimmed();

    if (v == kCodeFlushAll) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb(); refreshSessionRecordsView(); showStatusOk(QStringLiteral("記録完了"));
            m_model->clear(); m_source = ListSource::None; m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        }
        return;
    }
    if (v == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("计数器2 已清零。"), 1500);
        ui->lineEdit_4->clear(); return;
    }
    if (v == kCodeToSearch) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) { m_model->clear(); m_source = ListSource::None; }
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 搜索。"), 1500);
        return;
    }
    if (v == kCodeToRegister) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) { m_model->clear(); m_source = ListSource::None; }
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 入荷登録(前码)。"), 1500);
        return;
    }

    if (!ensureExactLenAndMark(ui->lineEdit_4, 13, ui->statusbar, QStringLiteral("仮登録(前码)")))
        return;

    ui->lineEdit_3->setFocus(); ui->lineEdit_3->selectAll();
}

void MainWindow::onTemp2Enter()
{
    const QString a = ui->lineEdit_4->text().trimmed();
    const QString b = ui->lineEdit_3->text().trimmed();

    if (b == kCodeFlushAll) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb(); refreshSessionRecordsView(); showStatusOk(QStringLiteral("記録完了"));
            m_model->clear(); m_source = ListSource::None; m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        }
        return;
    }
    if (b == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("计数器2 已清零。"), 1500);
        ui->lineEdit_3->clear(); return;
    }
    if (b == kCodeToSearch) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) { m_model->clear(); m_source = ListSource::None; }
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 仮登録。"), 1500);
        return;
    }
    if (b == kCodeToRegister) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) { m_model->clear(); m_source = ListSource::None; }
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 入荷登録(前码)。"), 1500);
        return;
    }

    if (!ensureExactLenAndMark(ui->lineEdit_3, 15, ui->statusbar, QStringLiteral("仮登録(后码)")))
        return;

    if (a.size() != 13) {
        ui->lineEdit_4->clear();
        ui->lineEdit_4->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("请先在 仮登録(前码) 输入 13 位。"), 2500);
        return;
    }

    const bool dupNow = existsInboundImeiInCurrentSession(b);
    QString hex; const QString disp = displayNameForJan(m_db, a, &hex);
    const QString left = disp.isEmpty() ? a : disp;
    const QString baseText = formatRecord(QStringLiteral("仮登録"), {left, b});

    if (dupNow) {
        // 重复：红字 + 彩点
        addToListWithSource(baseText, ListSource::Temp, QColor("#d32f2f"));
        if (m_model->rowCount() > 0) {
            auto* last = m_model->item(m_model->rowCount()-1);
            if (last) {
                last->setData(baseText, RoleBaseText);
                last->setData(a, RoleCode13);
                last->setData(b, RoleImei15);
                last->setData(QStringLiteral("仮登録"), RoleKind);
                setItemColorDot(last, hex);
            }
        }
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("仮登録：IMEI 重复，已标红（不会入库）。"), 2000);
        if (ui->label_8) {
            ui->label_8->setText(QStringLiteral("仮登録結果: %1（重複）").arg(left));
            ui->label_8->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }
    } else {
        // 正常：黑字 + 彩点
        addToListWithSource(baseText, ListSource::Temp /* no color */);
        if (m_model->rowCount() > 0) {
            auto* last = m_model->item(m_model->rowCount()-1);
            if (last) {
                last->setData(baseText, RoleBaseText);
                last->setData(a, RoleCode13);
                last->setData(b, RoleImei15);
                last->setData(QStringLiteral("仮登録"), RoleKind);
                setItemColorDot(last, hex);
            }
        }
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("仮登録：已加入。"), 1500);
        if (ui->label_8) {
            ui->label_8->setText(QStringLiteral("仮登録結果: %1").arg(left));
            ui->label_8->setStyleSheet(hex.isEmpty() ? QString()
                                                     : QStringLiteral("QLabel{ color:%1; }").arg(hex));
        }
    }

    renumberModel(m_model);
    ui->lineEdit_4->clear(); ui->lineEdit_3->clear(); ui->lineEdit_4->setFocus();
}

// ====================== 槽：検索 ======================
void MainWindow::onSearch1Enter()
{
    const QString v = ui->lineEdit_6->text().trimmed();

    if (v == kCodeFlushAll) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb(); refreshSessionRecordsView(); showStatusOk(QStringLiteral("記録完了"));
            m_model->clear(); m_source = ListSource::None; m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        }
        return;
    }
    if (v == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("计数器2 已清零。"), 1500);
        ui->lineEdit_6->clear(); return;
    }
    if (v == kCodeToRegister) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 入荷登録(前码)。"), 1500);
        return;
    }
    if (v == kCodeToTemp) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 仮登録。"), 1500);
        return;
    }

    if (!ensureExactLenAndMark(ui->lineEdit_6, 13, ui->statusbar, QStringLiteral("検索(前码)")))
        return;

    ui->lineEdit_5->setFocus(); ui->lineEdit_5->selectAll();
}

void MainWindow::onSearch2Enter()
{
    const QString a = ui->lineEdit_6->text().trimmed();
    const QString b = ui->lineEdit_5->text().trimmed();

    if (b == kCodeFlushAll) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb(); refreshSessionRecordsView(); showStatusOk(QStringLiteral("記録完了"));
            m_model->clear(); m_source = ListSource::None; m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        }
        return;
    }
    if (b == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("计数器2 已清零。"), 1500);
        ui->lineEdit_5->clear(); return;
    }
    if (b == kCodeToRegister) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 入荷登録(前码)。"), 1500);
        return;
    }
    if (b == kCodeToTemp) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("快捷码：跳到 仮登録。"), 1500);
        return;
    }

    if (!ensureExactLenAndMark(ui->lineEdit_5, 15, ui->statusbar, QStringLiteral("検索(后码)")))
        return;

    if (a.size() != 13) {
        ui->lineEdit_6->clear();
        ui->lineEdit_6->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("请先在 検索(前码) 输入 13 位。"), 2500);
        return;
    }

    // 业务：検索显示（机型名 + 彩点 + 序号）
    QString hex;
    const QString disp = displayNameForJan(m_db, a, &hex);
    const QString left = disp.isEmpty() ? a : disp;
    if (ui->label_13)
        ui->label_13->setText(QStringLiteral("検索結果: %1 %2").arg(left, b));

    const QString baseText = formatRecord(QStringLiteral("検索"), {left, b});
    addToListWithSource(baseText, ListSource::Search /* 黑字 */);
    if (m_model->rowCount() > 0) {
        if (auto* last = m_model->item(m_model->rowCount()-1)) {
            last->setData(baseText, RoleBaseText);
            last->setData(a, RoleCode13);
            last->setData(b, RoleImei15);
            last->setData(QStringLiteral("検索"), RoleKind);
            setItemColorDot(last, hex);
        }
    }
    renumberModel(m_model);

    if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("検索：已更新列表。"), 1500);

    ui->lineEdit_6->clear(); ui->lineEdit_5->clear(); ui->lineEdit_6->setFocus();
}

// ====================== 槽：Reset ======================
void MainWindow::onResetClicked()
{
    m_model->clear();
    for (QLineEdit* w : std::as_const(m_scannerEdits)) w->clear();
    if (ui->label_13) ui->label_13->setText(QStringLiteral("検索結果"));
    m_source = ListSource::None;

    m_lcd2Counter = 0;
    ui->lcdNumber_2->display(m_lcd2Counter);

    ui->lineEdit->setFocus();
    if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("已重置（计数器2 已清零）。"), 1500);
}
