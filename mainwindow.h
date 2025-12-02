#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QList>
#include <QSet>
#include <QHash>
#include <QRegularExpression>
#include <QtSql/QSqlDatabase>
#include <QColor>
#include <QVector>
#include <QString>

class QEvent;

namespace QXlsx { class Document; }

struct ExportRow {
    int      seq = 0;         // 番号（1 开始）
    QString  jan;             // 13位 JAN
    QString  productName;     // 机型名（“型号 容量 颜色”）
    QString  imei;            // 15位 IMEI
    int      qty = 1;         // 每行数量（默认 1）
    double   unitPrice = 0.0; // 单价（目前未知，默认 0）
};



class QLineEdit;
class QStandardItemModel;
class QResizeEvent;
class QGraphicsView;
class QStatusBar;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// ---- 焦点高亮：获得焦点的 QLineEdit 背景置绿 ----
class FocusHighlighter : public QObject
{
    Q_OBJECT
public:
    explicit FocusHighlighter(const QList<QLineEdit*>& targets, QObject* parent = nullptr);
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QList<QLineEdit*> m_targets;
    QHash<QLineEdit*, QString> m_defaultStyles;
    void clearAll();
};

// ---- 扫码输入限制：只有指定的 QLineEdit 接受数字/退格（不拦截 Enter）----
class ScannerOnlyGuard : public QObject
{
    Q_OBJECT
public:
    explicit ScannerOnlyGuard(const QList<QLineEdit*>& allowed, QObject* parent = nullptr);
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    QSet<QWidget*> m_allowed;
    bool isDigitOrBackspace(int key) const;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    // 入荷登録
    void onReg1Enter();   // lineEdit -> Enter -> 校验/跳转
    void onReg2Enter();   // lineEdit_2 -> Enter -> 入库/复制/计数/10条清空

    // 検索
    void onSearch1Enter(); // lineEdit_6 -> Enter -> 校验/跳转
    void onSearch2Enter(); // lineEdit_5 -> Enter -> label_13 & listView 规则5

    // 仮登録
    void onTemp1Enter();   // lineEdit_4 -> Enter -> 校验/跳转
    void onTemp2Enter();   // lineEdit_3 -> Enter -> 即时查重标红 & listView 规则6

    // 复位
    void onResetClicked();

    void exportToExcel();    // 生成 Excel
    void openLastExport();   // 打开最近一次导出文件


private:
    enum class ListSource { None, Search, Temp };

    // —— UI/模型 —— //
    Ui::MainWindow *ui;
    QStandardItemModel* m_model;         // 左侧 listView
    QStandardItemModel* m_modelSession;  // 右侧 listView_2（会话记录）
    QList<QLineEdit*>   m_scannerEdits;
    FocusHighlighter*   m_highlighter;
    ScannerOnlyGuard*   m_guard;
    ListSource          m_source = ListSource::None;
    int                 m_lcd2Counter = 0;   // lcdNumber_2 的本地计数器（0~9）

    // —— 数据库/会话 —— //
    QSqlDatabase m_db;
    QString      m_sessionId;
    QString      m_statusDefaultStyle;

    // 初始化
    void initConnections();
    void initValidators();

    // SVG
    void setSvgToView(QGraphicsView* view,
                      const QString& qrcPath,
                      const QString& elementId = QString(),
                      Qt::AspectRatioMode mode = Qt::KeepAspectRatio);

    // 列表处理
    QString formatRecord(const QString& prefix, const QString& parts) const;       // 拼接已规范化的文本
    QString formatRecord(const QString& prefix, const QStringList& parts) const;   // 重载
    void appendListDirect(const QString& text);                                    // 入荷登録专用：不改来源
    void addToListWithSource(const QString& text, ListSource src,
                             const QColor& fgColor = QColor());                     // 支持传入前景色（红色标重）

    // 状态栏绿色提示
    void showStatusOk(const QString& text);

    // 数据库
    bool initDatabase();
    bool ensureSchema();
    bool insertInboundRow(const QString& kind, const QString& code13, const QString& imei15, QString* errText = nullptr);
    bool insertEntryLogRow(const QString& type, const QString& leftCode, const QString& rightCode);
    bool existsInboundImeiInCurrentSession(const QString& imei15) const;
    int  countInboundRowsForSessionKind(const QString& kind) const;
    void updateLcdFromDb();
    bool hasTempInListView() const;
    bool flushAllListItemsToDb();  // 仮登録→入荷登録，逐条去重，重复标红

    // 会话持久化
    void chooseOrCreateSessionOnStartup();  // 启动弹窗 继续/新建
    QString generateSessionId() const;
    QString readLastSessionIdQSettings() const;
    void    writeLastSessionIdQSettings(const QString& sid);

    // 会话记录面板
    void refreshSessionRecordsView();
    // Excel/导出相关
    QString m_lastExportPath;                       // 最近一次导出的本地路径
    QVector<ExportRow> gatherCurrentSessionRows() const;
    bool writeExportedItemsSheet(QXlsx::Document& xlsx,
                                 const QVector<ExportRow>& rows,
                                 double* totalAmountOut);
    bool writeWs3Sheet(QXlsx::Document& xlsx,
                       const QVector<ExportRow>& rows);

    // 音频提醒
    void playSound(const QString& soundName);


};

#endif // MAINWINDOW_H
