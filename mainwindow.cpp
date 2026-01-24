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
#include <xlsxcellrange.h>
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
#include <QWindow>

#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QGuiApplication>
#include <QScreen>
#include <QSoundEffect>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// QXlsx
#include <xlsxdocument.h>
#include <xlsxformat.h>
#include <xlsxworksheet.h>
#include <xlsxworkbook.h>

#include <algorithm>  // std::max
#include <numeric>    // std::accumulate（你在合计里用了）

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

// ============ Other Products JAN -> Product Name Mapping ============
// iPad, Magic Keyboard, Apple Watch 等产品的 JAN 到商品名映射
static const QHash<QString, QString>& getOtherProductsJanMap() {
    static const QHash<QString, QString> map = []() {
        QHash<QString, QString> m;
        // === iPad (A16) ===
        m.insert("4549995560077", "iPad (A16) 11\" 128GB シルバー");
        m.insert("4549995560084", "iPad (A16) 11\" 128GB ブルー");
        m.insert("4549995560091", "iPad (A16) 11\" 128GB イエロー");
        m.insert("4549995560107", "iPad (A16) 11\" 128GB ピンク");
        m.insert("4549995560114", "iPad (A16) 11\" 256GB シルバー");
        m.insert("4549995560121", "iPad (A16) 11\" 256GB ブルー");
        m.insert("4549995560138", "iPad (A16) 11\" 256GB イエロー");
        m.insert("4549995560145", "iPad (A16) 11\" 256GB ピンク");
        m.insert("4549995560152", "iPad (A16) 11\" 512GB シルバー");
        m.insert("4549995560169", "iPad (A16) 11\" 512GB ブルー");
        m.insert("4549995560176", "iPad (A16) 11\" 512GB イエロー");
        m.insert("4549995560183", "iPad (A16) 11\" 512GB ピンク");
        m.insert("4549995560848", "iPad (A16) 11\" Cellular 128GB シルバー");
        m.insert("4549995560886", "iPad (A16) 11\" Cellular 128GB ブルー");
        m.insert("4549995560923", "iPad (A16) 11\" Cellular 128GB イエロー");
        m.insert("4549995560961", "iPad (A16) 11\" Cellular 128GB ピンク");
        m.insert("4549995561005", "iPad (A16) 11\" Cellular 256GB シルバー");
        m.insert("4549995561043", "iPad (A16) 11\" Cellular 256GB ブルー");
        m.insert("4549995561081", "iPad (A16) 11\" Cellular 256GB イエロー");
        m.insert("4549995561128", "iPad (A16) 11\" Cellular 256GB ピンク");
        m.insert("4549995561166", "iPad (A16) 11\" Cellular 512GB シルバー");
        m.insert("4549995561203", "iPad (A16) 11\" Cellular 512GB ブルー");
        m.insert("4549995561241", "iPad (A16) 11\" Cellular 512GB イエロー");
        m.insert("4549995561289", "iPad (A16) 11\" Cellular 512GB ピンク");
        // === iPad mini (A17 Pro) ===
        m.insert("4549995526486", "iPad mini (A17 Pro) 128GB スペースグレイ");
        m.insert("4549995526493", "iPad mini (A17 Pro) 128GB ブルー");
        m.insert("4549995526509", "iPad mini (A17 Pro) 128GB スターライト");
        m.insert("4549995526516", "iPad mini (A17 Pro) 128GB パープル");
        m.insert("4549995526523", "iPad mini (A17 Pro) 256GB スペースグレイ");
        m.insert("4549995526530", "iPad mini (A17 Pro) 256GB ブルー");
        m.insert("4549995526547", "iPad mini (A17 Pro) 256GB スターライト");
        m.insert("4549995526554", "iPad mini (A17 Pro) 256GB パープル");
        m.insert("4549995530537", "iPad mini (A17 Pro) 512GB スペースグレイ");
        m.insert("4549995530544", "iPad mini (A17 Pro) 512GB ブルー");
        m.insert("4549995530551", "iPad mini (A17 Pro) 512GB スターライト");
        m.insert("4549995530568", "iPad mini (A17 Pro) 512GB パープル");
        m.insert("4549995526769", "iPad mini (A17 Pro) Cellular 128GB スペースグレイ");
        m.insert("4549995526776", "iPad mini (A17 Pro) Cellular 128GB ブルー");
        m.insert("4549995526783", "iPad mini (A17 Pro) Cellular 128GB スターライト");
        m.insert("4549995526790", "iPad mini (A17 Pro) Cellular 128GB パープル");
        m.insert("4549995526806", "iPad mini (A17 Pro) Cellular 256GB スペースグレイ");
        m.insert("4549995526813", "iPad mini (A17 Pro) Cellular 256GB ブルー");
        m.insert("4549995526820", "iPad mini (A17 Pro) Cellular 256GB スターライト");
        m.insert("4549995526837", "iPad mini (A17 Pro) Cellular 256GB パープル");
        m.insert("4549995530698", "iPad mini (A17 Pro) Cellular 512GB スペースグレイ");
        m.insert("4549995530704", "iPad mini (A17 Pro) Cellular 512GB ブルー");
        m.insert("4549995530711", "iPad mini (A17 Pro) Cellular 512GB スターライト");
        m.insert("4549995530728", "iPad mini (A17 Pro) Cellular 512GB パープル");
        // === iPad Air (M3) 11" ===
        m.insert("4549995555233", "iPad Air (M3) 11\" 128GB スペースグレイ");
        m.insert("4549995555240", "iPad Air (M3) 11\" 128GB ブルー");
        m.insert("4549995555257", "iPad Air (M3) 11\" 128GB スターライト");
        m.insert("4549995555264", "iPad Air (M3) 11\" 128GB パープル");
        m.insert("4549995555271", "iPad Air (M3) 11\" 256GB スペースグレイ");
        m.insert("4549995555288", "iPad Air (M3) 11\" 256GB ブルー");
        m.insert("4549995555295", "iPad Air (M3) 11\" 256GB スターライト");
        m.insert("4549995555301", "iPad Air (M3) 11\" 256GB パープル");
        m.insert("4549995555318", "iPad Air (M3) 11\" 512GB スペースグレイ");
        m.insert("4549995555325", "iPad Air (M3) 11\" 512GB ブルー");
        m.insert("4549995555332", "iPad Air (M3) 11\" 512GB スターライト");
        m.insert("4549995555349", "iPad Air (M3) 11\" 512GB パープル");
        m.insert("4549995555356", "iPad Air (M3) 11\" 1TB スペースグレイ");
        m.insert("4549995555363", "iPad Air (M3) 11\" 1TB ブルー");
        m.insert("4549995555370", "iPad Air (M3) 11\" 1TB スターライト");
        m.insert("4549995555387", "iPad Air (M3) 11\" 1TB パープル");
        m.insert("4549995555752", "iPad Air (M3) 11\" Cellular 128GB スペースグレイ");
        m.insert("4549995555769", "iPad Air (M3) 11\" Cellular 128GB ブルー");
        m.insert("4549995555776", "iPad Air (M3) 11\" Cellular 128GB スターライト");
        m.insert("4549995555783", "iPad Air (M3) 11\" Cellular 128GB パープル");
        m.insert("4549995555790", "iPad Air (M3) 11\" Cellular 256GB スペースグレイ");
        m.insert("4549995555806", "iPad Air (M3) 11\" Cellular 256GB ブルー");
        m.insert("4549995555813", "iPad Air (M3) 11\" Cellular 256GB スターライト");
        m.insert("4549995555820", "iPad Air (M3) 11\" Cellular 256GB パープル");
        m.insert("4549995555837", "iPad Air (M3) 11\" Cellular 512GB スペースグレイ");
        m.insert("4549995555844", "iPad Air (M3) 11\" Cellular 512GB ブルー");
        m.insert("4549995555851", "iPad Air (M3) 11\" Cellular 512GB スターライト");
        m.insert("4549995555868", "iPad Air (M3) 11\" Cellular 512GB パープル");
        m.insert("4549995555875", "iPad Air (M3) 11\" Cellular 1TB スペースグレイ");
        m.insert("4549995555882", "iPad Air (M3) 11\" Cellular 1TB ブルー");
        m.insert("4549995555899", "iPad Air (M3) 11\" Cellular 1TB スターライト");
        m.insert("4549995555905", "iPad Air (M3) 11\" Cellular 1TB パープル");
        // === iPad Air (M3) 13" ===
        m.insert("4549995556315", "iPad Air (M3) 13\" 128GB スペースグレイ");
        m.insert("4549995556322", "iPad Air (M3) 13\" 128GB ブルー");
        m.insert("4549995556339", "iPad Air (M3) 13\" 128GB スターライト");
        m.insert("4549995556346", "iPad Air (M3) 13\" 128GB パープル");
        m.insert("4549995556353", "iPad Air (M3) 13\" 256GB スペースグレイ");
        m.insert("4549995556360", "iPad Air (M3) 13\" 256GB ブルー");
        m.insert("4549995556377", "iPad Air (M3) 13\" 256GB スターライト");
        m.insert("4549995556384", "iPad Air (M3) 13\" 256GB パープル");
        m.insert("4549995556391", "iPad Air (M3) 13\" 512GB スペースグレイ");
        m.insert("4549995556407", "iPad Air (M3) 13\" 512GB ブルー");
        m.insert("4549995556414", "iPad Air (M3) 13\" 512GB スターライト");
        m.insert("4549995556421", "iPad Air (M3) 13\" 512GB パープル");
        m.insert("4549995556438", "iPad Air (M3) 13\" 1TB スペースグレイ");
        m.insert("4549995556445", "iPad Air (M3) 13\" 1TB ブルー");
        m.insert("4549995556452", "iPad Air (M3) 13\" 1TB スターライト");
        m.insert("4549995556469", "iPad Air (M3) 13\" 1TB パープル");
        m.insert("4549995556834", "iPad Air (M3) 13\" Cellular 128GB スペースグレイ");
        m.insert("4549995556841", "iPad Air (M3) 13\" Cellular 128GB ブルー");
        m.insert("4549995556858", "iPad Air (M3) 13\" Cellular 128GB スターライト");
        m.insert("4549995556865", "iPad Air (M3) 13\" Cellular 128GB パープル");
        m.insert("4549995556872", "iPad Air (M3) 13\" Cellular 256GB スペースグレイ");
        m.insert("4549995556889", "iPad Air (M3) 13\" Cellular 256GB ブルー");
        m.insert("4549995556896", "iPad Air (M3) 13\" Cellular 256GB スターライト");
        m.insert("4549995556902", "iPad Air (M3) 13\" Cellular 256GB パープル");
        m.insert("4549995556919", "iPad Air (M3) 13\" Cellular 512GB スペースグレイ");
        m.insert("4549995556926", "iPad Air (M3) 13\" Cellular 512GB ブルー");
        m.insert("4549995556933", "iPad Air (M3) 13\" Cellular 512GB スターライト");
        m.insert("4549995556940", "iPad Air (M3) 13\" Cellular 512GB パープル");
        m.insert("4549995556957", "iPad Air (M3) 13\" Cellular 1TB スペースグレイ");
        m.insert("4549995556964", "iPad Air (M3) 13\" Cellular 1TB ブルー");
        m.insert("4549995556971", "iPad Air (M3) 13\" Cellular 1TB スターライト");
        m.insert("4549995556988", "iPad Air (M3) 13\" Cellular 1TB パープル");
        // === iPad Pro (M5) 11" ===
        m.insert("4549995616606", "iPad Pro (M5) 11\" 256GB スペースブラック");
        m.insert("4549995616613", "iPad Pro (M5) 11\" 256GB シルバー");
        m.insert("4549995616620", "iPad Pro (M5) 11\" 512GB スペースブラック");
        m.insert("4549995616637", "iPad Pro (M5) 11\" 512GB シルバー");
        m.insert("4549995616644", "iPad Pro (M5) 11\" 1TB スペースブラック");
        m.insert("4549995616651", "iPad Pro (M5) 11\" 1TB シルバー");
        m.insert("4549995616682", "iPad Pro (M5) 11\" 2TB スペースブラック");
        m.insert("4549995616699", "iPad Pro (M5) 11\" 2TB シルバー");
        m.insert("4549995616668", "iPad Pro (M5) 11\" 1TB Nano スペースブラック");
        m.insert("4549995616675", "iPad Pro (M5) 11\" 1TB Nano シルバー");
        m.insert("4549995616705", "iPad Pro (M5) 11\" 2TB Nano スペースブラック");
        m.insert("4549995616712", "iPad Pro (M5) 11\" 2TB Nano シルバー");
        m.insert("4549995616842", "iPad Pro (M5) 11\" Cellular 256GB スペースブラック");
        m.insert("4549995616880", "iPad Pro (M5) 11\" Cellular 256GB シルバー");
        m.insert("4549995616927", "iPad Pro (M5) 11\" Cellular 512GB スペースブラック");
        m.insert("4549995616965", "iPad Pro (M5) 11\" Cellular 512GB シルバー");
        m.insert("4549995617009", "iPad Pro (M5) 11\" Cellular 1TB スペースブラック");
        m.insert("4549995617047", "iPad Pro (M5) 11\" Cellular 1TB シルバー");
        m.insert("4549995617160", "iPad Pro (M5) 11\" Cellular 2TB スペースブラック");
        m.insert("4549995617207", "iPad Pro (M5) 11\" Cellular 2TB シルバー");
        m.insert("4549995617085", "iPad Pro (M5) 11\" Cellular 1TB Nano スペースブラック");
        m.insert("4549995617122", "iPad Pro (M5) 11\" Cellular 1TB Nano シルバー");
        m.insert("4549995617245", "iPad Pro (M5) 11\" Cellular 2TB Nano スペースブラック");
        m.insert("4549995617283", "iPad Pro (M5) 11\" Cellular 2TB Nano シルバー");
        // === iPad Pro (M5) 13" ===
        m.insert("4549995616729", "iPad Pro (M5) 13\" 256GB スペースブラック");
        m.insert("4549995616736", "iPad Pro (M5) 13\" 256GB シルバー");
        m.insert("4549995616743", "iPad Pro (M5) 13\" 512GB スペースブラック");
        m.insert("4549995616750", "iPad Pro (M5) 13\" 512GB シルバー");
        m.insert("4549995616767", "iPad Pro (M5) 13\" 1TB スペースブラック");
        m.insert("4549995616774", "iPad Pro (M5) 13\" 1TB シルバー");
        m.insert("4549995616804", "iPad Pro (M5) 13\" 2TB スペースブラック");
        m.insert("4549995616811", "iPad Pro (M5) 13\" 2TB シルバー");
        m.insert("4549995616781", "iPad Pro (M5) 13\" 1TB Nano スペースブラック");
        m.insert("4549995616798", "iPad Pro (M5) 13\" 1TB Nano シルバー");
        m.insert("4549995616828", "iPad Pro (M5) 13\" 2TB Nano スペースブラック");
        m.insert("4549995616835", "iPad Pro (M5) 13\" 2TB Nano シルバー");
        m.insert("4549995617320", "iPad Pro (M5) 13\" Cellular 256GB スペースブラック");
        m.insert("4549995617368", "iPad Pro (M5) 13\" Cellular 256GB シルバー");
        m.insert("4549995617405", "iPad Pro (M5) 13\" Cellular 512GB スペースブラック");
        m.insert("4549995617443", "iPad Pro (M5) 13\" Cellular 512GB シルバー");
        m.insert("4549995617481", "iPad Pro (M5) 13\" Cellular 1TB スペースブラック");
        m.insert("4549995617528", "iPad Pro (M5) 13\" Cellular 1TB シルバー");
        m.insert("4549995617641", "iPad Pro (M5) 13\" Cellular 2TB スペースブラック");
        m.insert("4549995617689", "iPad Pro (M5) 13\" Cellular 2TB シルバー");
        m.insert("4549995617566", "iPad Pro (M5) 13\" Cellular 1TB Nano スペースブラック");
        m.insert("4549995617603", "iPad Pro (M5) 13\" Cellular 1TB Nano シルバー");
        m.insert("4549995617726", "iPad Pro (M5) 13\" Cellular 2TB Nano スペースブラック");
        m.insert("4549995617764", "iPad Pro (M5) 13\" Cellular 2TB Nano シルバー");
        // === Magic Keyboard ===
        m.insert("4549995498158", "Magic Keyboard (iPad Pro) 11\" White JP");
        m.insert("4549995504811", "Magic Keyboard (iPad Pro) 11\" White US");
        m.insert("4549995504415", "Magic Keyboard (iPad Pro) 11\" White UK");
        m.insert("4549995504774", "Magic Keyboard (iPad Pro) 11\" White CN");
        m.insert("4549995504699", "Magic Keyboard (iPad Pro) 11\" White TW");
        m.insert("4549995504736", "Magic Keyboard (iPad Pro) 11\" White KR");
        m.insert("4549995498165", "Magic Keyboard (iPad Pro) 11\" Black JP");
        m.insert("4549995504828", "Magic Keyboard (iPad Pro) 11\" Black US");
        m.insert("4549995504422", "Magic Keyboard (iPad Pro) 11\" Black UK");
        m.insert("4549995498172", "Magic Keyboard (iPad Pro) 13\" White JP");
        m.insert("4549995498189", "Magic Keyboard (iPad Pro) 13\" Black JP");
        m.insert("4549995567588", "Magic Keyboard (iPad Air) 11\" White JP");
        m.insert("4549995613575", "Magic Keyboard (iPad Air) 11\" White US");
        m.insert("4549995613551", "Magic Keyboard (iPad Air) 11\" White CN");
        m.insert("4549995613513", "Magic Keyboard (iPad Air) 11\" White TW");
        m.insert("4549995613537", "Magic Keyboard (iPad Air) 11\" White KR");
        m.insert("4549995613490", "Magic Keyboard (iPad Air) 11\" White ES");
        m.insert("4549995567595", "Magic Keyboard (iPad Air) 13\" White JP");
        m.insert("4549995660203", "Magic Keyboard (iPad Air) 13\" Black JP");
        m.insert("4549995364415", "Magic Keyboard Folio White JP");
        // === Apple Watch Series 11 GPS 42mm ===
        m.insert("4549995623345", "Watch S11 GPS 42mm ジェットブラック S/M");
        m.insert("4549995622720", "Watch S11 GPS 42mm ジェットブラック M/L");
        m.insert("4549995622768", "Watch S11 GPS 42mm スペースグレイ S/M");
        m.insert("4549995622782", "Watch S11 GPS 42mm スペースグレイ M/L");
        m.insert("4549995622829", "Watch S11 GPS 42mm ローズゴールド S/M");
        m.insert("4549995622843", "Watch S11 GPS 42mm ローズゴールド M/L");
        m.insert("4549995622881", "Watch S11 GPS 42mm シルバー S/M");
        m.insert("4549995622904", "Watch S11 GPS 42mm シルバー M/L");
        // === Apple Watch Series 11 GPS 46mm ===
        m.insert("4549995622942", "Watch S11 GPS 46mm ジェットブラック S/M");
        m.insert("4549995622966", "Watch S11 GPS 46mm ジェットブラック M/L");
        m.insert("4549995623000", "Watch S11 GPS 46mm スペースグレイ S/M");
        m.insert("4549995623024", "Watch S11 GPS 46mm スペースグレイ M/L");
        m.insert("4549995623062", "Watch S11 GPS 46mm ローズゴールド S/M");
        m.insert("4549995623086", "Watch S11 GPS 46mm ローズゴールド M/L");
        m.insert("4549995623123", "Watch S11 GPS 46mm シルバー S/M");
        m.insert("4549995623147", "Watch S11 GPS 46mm シルバー M/L");
        // === Apple Watch Series 11 Cellular 42mm Aluminum ===
        m.insert("4549995625837", "Watch S11 Cel 42mm ジェットブラック S/M");
        m.insert("4549995625851", "Watch S11 Cel 42mm ジェットブラック M/L");
        m.insert("4549995625899", "Watch S11 Cel 42mm スペースグレイ S/M");
        m.insert("4549995625912", "Watch S11 Cel 42mm スペースグレイ M/L");
        m.insert("4549995625950", "Watch S11 Cel 42mm ローズゴールド S/M");
        m.insert("4549995625974", "Watch S11 Cel 42mm ローズゴールド M/L");
        m.insert("4549995626018", "Watch S11 Cel 42mm シルバー S/M");
        m.insert("4549995626032", "Watch S11 Cel 42mm シルバー M/L");
        // === Apple Watch Series 11 Cellular 46mm Aluminum ===
        m.insert("4549995626599", "Watch S11 Cel 46mm ジェットブラック S/M");
        m.insert("4549995626636", "Watch S11 Cel 46mm ジェットブラック M/L");
        m.insert("4549995626698", "Watch S11 Cel 46mm スペースグレイ S/M");
        m.insert("4549995626711", "Watch S11 Cel 46mm スペースグレイ M/L");
        m.insert("4549995626759", "Watch S11 Cel 46mm ローズゴールド S/M");
        m.insert("4549995626773", "Watch S11 Cel 46mm ローズゴールド M/L");
        m.insert("4549995626810", "Watch S11 Cel 46mm シルバー S/M");
        m.insert("4549995626834", "Watch S11 Cel 46mm シルバー M/L");
        // === Apple Watch Series 11 Cellular 42mm Titanium ===
        m.insert("4549995626070", "Watch S11 Ti 42mm ナチュラル S/M");
        m.insert("4549995626094", "Watch S11 Ti 42mm ナチュラル M/L");
        m.insert("4549995626117", "Watch S11 Ti 42mm ナチュラル ミラネーゼ");
        m.insert("4549995626155", "Watch S11 Ti 42mm スレート S/M");
        m.insert("4549995626179", "Watch S11 Ti 42mm スレート M/L");
        m.insert("4549995626193", "Watch S11 Ti 42mm スレート ミラネーゼ");
        m.insert("4549995626230", "Watch S11 Ti 42mm ゴールド S/M");
        m.insert("4549995626254", "Watch S11 Ti 42mm ゴールド M/L");
        m.insert("4549995626278", "Watch S11 Ti 42mm ゴールド ミラネーゼ");
        // === Apple Watch Series 11 Cellular 46mm Titanium ===
        m.insert("4549995626872", "Watch S11 Ti 46mm ナチュラル S/M");
        m.insert("4549995626896", "Watch S11 Ti 46mm ナチュラル M/L");
        m.insert("4549995626919", "Watch S11 Ti 46mm ナチュラル ミラネーゼ S/M");
        m.insert("4549995626933", "Watch S11 Ti 46mm ナチュラル ミラネーゼ M/L");
        m.insert("4549995626957", "Watch S11 Ti 46mm スレート S/M");
        m.insert("4549995626971", "Watch S11 Ti 46mm スレート M/L");
        m.insert("4549995626995", "Watch S11 Ti 46mm スレート ミラネーゼ S/M");
        m.insert("4549995627015", "Watch S11 Ti 46mm スレート ミラネーゼ M/L");
        m.insert("4549995627039", "Watch S11 Ti 46mm ゴールド S/M");
        m.insert("4549995627053", "Watch S11 Ti 46mm ゴールド M/L");
        m.insert("4549995627077", "Watch S11 Ti 46mm ゴールド ミラネーゼ S/M");
        m.insert("4549995627091", "Watch S11 Ti 46mm ゴールド ミラネーゼ M/L");
        // === Apple Watch SE 3 GPS ===
        m.insert("4549995615388", "Watch SE3 GPS 40mm スターライト S/M");
        m.insert("4549995615395", "Watch SE3 GPS 40mm スターライト M/L");
        m.insert("4549995615418", "Watch SE3 GPS 40mm ミッドナイト S/M");
        m.insert("4549995615425", "Watch SE3 GPS 40mm ミッドナイト M/L");
        m.insert("4549995615449", "Watch SE3 GPS 44mm スターライト S/M");
        m.insert("4549995615456", "Watch SE3 GPS 44mm スターライト M/L");
        m.insert("4549995615470", "Watch SE3 GPS 44mm ミッドナイト S/M");
        m.insert("4549995615487", "Watch SE3 GPS 44mm ミッドナイト M/L");
        // === Apple Watch SE 3 Cellular ===
        m.insert("4549995618761", "Watch SE3 Cel 40mm スターライト S/M");
        m.insert("4549995618808", "Watch SE3 Cel 40mm スターライト M/L");
        m.insert("4549995618884", "Watch SE3 Cel 40mm ミッドナイト S/M");
        m.insert("4549995618921", "Watch SE3 Cel 40mm ミッドナイト M/L");
        m.insert("4549995619003", "Watch SE3 Cel 44mm スターライト S/M");
        m.insert("4549995619041", "Watch SE3 Cel 44mm スターライト M/L");
        m.insert("4549995619126", "Watch SE3 Cel 44mm ミッドナイト S/M");
        m.insert("4549995619164", "Watch SE3 Cel 44mm ミッドナイト M/L");
        // === Apple Watch Ultra 3 ===
        m.insert("4549995629477", "Watch Ultra 3 オーシャン アンカーブルー");
        m.insert("4549995629514", "Watch Ultra 3 オーシャン ブラック");
        m.insert("4549995629552", "Watch Ultra 3 アルパイン ライトブルー S");
        m.insert("4549995629576", "Watch Ultra 3 アルパイン ライトブルー M");
        m.insert("4549995629590", "Watch Ultra 3 アルパイン ライトブルー L");
        m.insert("4549995629637", "Watch Ultra 3 アルパイン ブラック S");
        m.insert("4549995629675", "Watch Ultra 3 アルパイン ブラック M");
        m.insert("4549995629699", "Watch Ultra 3 アルパイン ブラック L");
        m.insert("4549995629736", "Watch Ultra 3 トレイル ナチュラル/ブルー S/M");
        m.insert("4549995629750", "Watch Ultra 3 トレイル ブライトブルー M/L");
        m.insert("4549995629798", "Watch Ultra 3 トレイル ブラック/チャコール S/M");
        m.insert("4549995629835", "Watch Ultra 3 トレイル ブラック/チャコール M/L");
        m.insert("4549995629873", "Watch Ultra 3 Tiミラネーゼ ナチュラル S");
        m.insert("4549995629897", "Watch Ultra 3 Tiミラネーゼ ナチュラル M");
        m.insert("4549995629934", "Watch Ultra 3 Tiミラネーゼ ナチュラル L");
        m.insert("4549995629972", "Watch Ultra 3 Tiミラネーゼ ブラック S");
        m.insert("4549995629996", "Watch Ultra 3 Tiミラネーゼ ブラック M");
        m.insert("4549995630039", "Watch Ultra 3 Tiミラネーゼ ブラック L");
        return m;
    }();
    return map;
}

// 查询 JAN 对应的商品名（优先查 Other Products 映射表）
static QString productNameForJan(const QString& jan) {
    const auto& map = getOtherProductsJanMap();
    auto it = map.find(jan);
    if (it != map.end()) {
        return it.value();
    }
    return QString();  // 未找到
}

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

// —— 获取 DPI 缩放因子 —— //
static qreal dpiScaleFactorFor(QWidget *w) {
    QScreen *screen = nullptr;
    if (w && w->window() && w->window()->windowHandle())
        screen = w->window()->windowHandle()->screen();

    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (!screen)
        return 1.0;   // 兜底

    // 以 96 DPI 作为"100%"的基准
    return screen->logicalDotsPerInch() / 96.0;
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
static void setItemColorDot(QStandardItem* it, const QString& hex, QWidget *forDpiWidget) {
    if (!it) return;
    if (hex.isEmpty()) {
        it->setData(QVariant(), Qt::DecorationRole);
        return;
    }

    const qreal scale = dpiScaleFactorFor(forDpiWidget);
    const int diameter = static_cast<int>(18 * scale + 0.5); // 基准 18 像素，根据 DPI 放大

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
                       double baseZoom = 1.3,
                       Qt::AspectRatioMode mode = Qt::KeepAspectRatio)
{
    if (!view || !view->scene() || view->scene()->items().isEmpty()) return;

    view->resetTransform();
    view->fitInView(view->scene()->itemsBoundingRect(), mode);

    // 根据 DPI 叠加一层缩放，比如 125% 时大约是 1.25
    const qreal dpiScale = dpiScaleFactorFor(view);
    const qreal zoom = baseZoom * dpiScale;  // 100%≈1.3, 125%≈1.3×1.25

    if (zoom > 1.0)
        view->scale(zoom, zoom);
}

// —— 特殊码 —— //
static const QString kCodeToSearch   = QStringLiteral("2222222222222");   // 跳到 検索(lineEdit_6)
static const QString kCodeToRegister = QStringLiteral("5555555555555");   // 入荷登録：搜索/仮登録 -> lineEdit
static const QString kCodeToTemp     = QStringLiteral("3333333333333");   // 跳到 仮登録(lineEdit_4)
static const QString kCodeResetCount = QStringLiteral("1111111111111");   // 计数器2清零
static const QString kCodeFlushAll   = QStringLiteral("4444444444444");   // 仮登録列表批量落库
static const QString kCodeExcelExport = QStringLiteral("7777777777777"); // Excel出力
static const QString kCodeExcelOpen   = QStringLiteral("8888888888888"); // Excel表示

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
    if (bar) bar->showMessage(QStringLiteral("%1 には %2 桁数が必要，クリアしました。").arg(name).arg(expected), 2500);
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
    , m_modelTab2(new QStandardItemModel(this))
    , m_highlighter(nullptr)
    , m_guard(nullptr)
    , m_tab2Counter(0)
    , m_networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // SVG：加载到 8 个 graphicsView_*
    setSvgToView(ui->graphicsView_6,  QStringLiteral(":/Reset.svg"));
    setSvgToView(ui->graphicsView_7,  QStringLiteral(":/Search.svg"));
    setSvgToView(ui->graphicsView_8,  QStringLiteral(":/Stock_Registration.svg"));
    setSvgToView(ui->graphicsView_9,  QStringLiteral(":/Temporary_Registration.svg"));
    setSvgToView(ui->graphicsView_10, QStringLiteral(":/Temporary_Registration_Write.svg"));
    setSvgToView(ui->graphicsView_12, QStringLiteral(":/Excel_Output.svg"));
    setSvgToView(ui->graphicsView_13, QStringLiteral(":/Excel_Display.svg"));

    // listView 绑定模型（左/右）+ 设置图标尺寸（大圆点）
    ui->listView->setModel(m_model);
    ui->listView_2->setModel(m_modelSession);
    ui->listView_3->setModel(m_modelTab2);  // tab_2 的 listView_3

    // 根据 DPI 设置图标尺寸
    const qreal iconScale = dpiScaleFactorFor(this);
    const int iconD = static_cast<int>(18 * iconScale + 0.5);

    ui->listView->setIconSize(QSize(iconD, iconD));
    ui->listView_2->setIconSize(QSize(iconD, iconD));
    ui->listView_3->setIconSize(QSize(iconD, iconD));

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
        ui->statusbar->showMessage(QStringLiteral("データベースの初期化に失敗しました。権限/パスを確認してください。"), 4000);

    // 会话选择（继续上次 / 新建）
    chooseOrCreateSessionOnStartup();

    // 记住状态栏默认样式
    m_statusDefaultStyle = ui->statusbar ? ui->statusbar->styleSheet() : QString();

    // 同步计数与右侧会话列表
    updateLcdFromDb();
    refreshSessionRecordsView();

    // tab_2: 初始化计数器和从数据库刷新listView_3
    ui->lcdNumber_3->display(0);
    refreshTab2ListView();

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

// ====================== 音频提醒 ======================
void MainWindow::playSound(const QString& soundName)
{
    // 资源内部路径（给 QFile 检查用）
    const QString qrcFilePath = QStringLiteral(":/sounds/%1.wav").arg(soundName);
    // QSoundEffect 需要的 URL（qrc: scheme）
    const QString urlPath     = QStringLiteral("qrc:/sounds/%1.wav").arg(soundName);

    // 先确认资源真的在 qrc 里
    if (!QFile::exists(qrcFilePath)) {
        qDebug() << "[SOUND] qrc file NOT found:" << qrcFilePath;
        return;
    }

    auto *effect = new QSoundEffect(this);

    // 调试：看一下状态变化
    connect(effect, &QSoundEffect::statusChanged, this, [effect]() {
        qDebug() << "[SOUND] status =" << effect->status()
        << "source =" << effect->source();
    });

    effect->setSource(QUrl(urlPath));   // 关键：使用 "qrc:/..." 而不是 ":/..."
    effect->setLoopCount(1);
    effect->setVolume(0.7);

    // 播放结束后自动删除
    connect(effect, &QSoundEffect::playingChanged, this, [effect]() {
        if (!effect->isPlaying()) {
            effect->deleteLater();
        }
    });

    qDebug() << "[SOUND] try play:" << urlPath;
    effect->play();
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

    // tab_2: plainTextEdit 的 Enter 键检测
    if (obj == ui->plainTextEdit && e->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onPlainTextEnter();
            return true;  // 吞掉事件，不插入换行
        }
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

    // 先清理同名 Sheet，避免重复
    if (xlsx.sheetNames().contains("Exported_Items"))
        xlsx.deleteSheet("Exported_Items");
    xlsx.addSheet("Exported_Items");
    xlsx.selectSheet("Exported_Items");

    // —— 聚合（按 JAN）——
    struct Agg { QString product; int qty = 0; };
    QMap<QString, Agg> agg;
    for (const auto& e : rows) {
        auto &a = agg[e.jan];
        if (a.product.isEmpty()) {
            QString hex; const QString disp = displayNameForJan(m_db, e.jan, &hex);
            a.product = disp.isEmpty() ? e.jan : disp;
        }
        a.qty += e.qty; // 逐条=1
    }
    const int totalQty = std::accumulate(agg.cbegin(), agg.cend(), 0,
                                         [](int s, const Agg& a){ return s + a.qty; });
    const double totalAmount = 0.0; // 目前单价未知 → 金额先置 0（仅用于返回指针）

    // —— 样式 —— //
    Format top;      top.setFontBold(true); top.setBorderStyle(Format::BorderMedium);
    top.setHorizontalAlignment(Format::AlignHCenter); top.setVerticalAlignment(Format::AlignVCenter);

    Format th;       th.setFontBold(true); th.setBorderStyle(Format::BorderMedium);
    th.setHorizontalAlignment(Format::AlignHCenter); th.setVerticalAlignment(Format::AlignVCenter);

    Format tdL;      tdL.setBorderStyle(Format::BorderThin);
    Format tdC = tdL; tdC.setHorizontalAlignment(Format::AlignHCenter);
    Format tdR = tdL; tdR.setHorizontalAlignment(Format::AlignRight);

    Format noborder; // 用于“日付/ご署名”
    noborder.setBorderStyle(Format::BorderNone);

    auto mergeAndWrite = [&](int r1,int c1,int r2,int c2,const QVariant& v,const Format& f){
        xlsx.mergeCells(CellRange(r1,c1,r2,c2), f);
        xlsx.write(r1,c1,v,f);
    };

    int r = 1;

    // 顶部合计：A1-B1=合計（合并），C1=件数，E1-F1=合計金額（合并），G1=金额(后面填公式)
    mergeAndWrite(1, 1, 1, 2, QStringLiteral("合計"), top);      // A1-B1
    xlsx.write(1, 3, totalQty, top);                             // C1
    mergeAndWrite(1, 5, 1, 6, QStringLiteral("合計金額"), top);  // E1-F1
    // ★ G1 先不写，汇总数据范围确定后再写 SUM 公式

    r = 3;

    // 汇总表表头（第3行）：A-B:JAN(合并), C:Product, D:(空), E:Qty, F:Unit Price, G:Total Amount
    mergeAndWrite(r, 1, r, 2, "JAN", th);                        // A-B
    xlsx.write(r, 3, "Product",      th);                        // C
    xlsx.write(r, 4, "",             th);                        // D（空但上边框）
    xlsx.write(r, 5, "Qty",          th);                        // E
    xlsx.write(r, 6, "Unit Price",   th);                        // F
    xlsx.write(r, 7, "Total Amount", th);                        // G
    ++r;

    const int summaryDataStartRow = r;                           // ★ 汇总数据起始行(通常=4)

    // 汇总表数据
    for (auto it = agg.cbegin(); it != agg.cend(); ++it, ++r) {
        xlsx.mergeCells(QXlsx::CellRange(r, 1, r, 2), tdL);
        xlsx.write(r, 1, it.key(),           tdL);  // A
        xlsx.write(r, 2, "",                 tdL);  // B（与表头合并风格对应）
        xlsx.write(r, 3, it.value().product, tdL);  // C
        xlsx.write(r, 4, "",                 tdL);  // D（空白列也画边框）
        xlsx.write(r, 5, it.value().qty,     tdC);  // E: Qty

        // F: Unit Price，先写 0，之后用户在 Excel 中手动输入单价
        xlsx.write(r, 6, 0, tdC);                    // F

        // ★ G: Total Amount = F<ROW> * E<ROW>
        const QString totalFormula = QStringLiteral("=F%1*E%1").arg(r);
        xlsx.write(r, 7, totalFormula, tdR);         // G
    }

    // ★ 顶部 G1 = SUM(G<起始行>:G<结束行>)
    const int summaryDataEndRow = r - 1;
    if (summaryDataEndRow >= summaryDataStartRow) {
        const QString totalSumFormula =
            QStringLiteral("=SUM(G%1:G%2)")
                .arg(summaryDataStartRow)
                .arg(summaryDataEndRow);
        xlsx.write(1, 7, totalSumFormula, top);      // G1: 合計金額
    } else {
        xlsx.write(1, 7, 0, top);                    // 没有数据就保持为 0
    }

    // 空一行
    ++r;

    // 明细表表头：A:番号, B:JANコード, C:型番, D:空, E:数量, F-G:IMEI番号(合并)
    xlsx.write(r, 1, QStringLiteral("番号"),      th);
    xlsx.write(r, 2, QStringLiteral("JANコード"), th);
    xlsx.write(r, 3, QStringLiteral("型番"),      th);
    xlsx.write(r, 4, "",                          th); // D 空
    xlsx.write(r, 5, QStringLiteral("数量"),      th);
    mergeAndWrite(r, 6, r, 7, QStringLiteral("IMEI番号"), th);
    ++r;

    // 明细行（逐台）
    for (const auto& e : rows) {
        xlsx.write(r, 1, e.seq,         tdC);
        xlsx.write(r, 2, e.jan,         tdL);
        xlsx.write(r, 3, e.productName, tdL);
        xlsx.write(r, 4, "",            tdL);  // D 空
        xlsx.write(r, 5, e.qty,         tdC);
        mergeAndWrite(r, 6, r, 7, e.imei, tdL);
        ++r;
    }

    // 空一行
    ++r;

    // 日付 / ご署名（无边框，且签名在 E 列）
    xlsx.write(r, 2, QStringLiteral("日付：%1")
                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")), noborder);
    xlsx.write(r, 5, QStringLiteral("ご署名："), noborder);

    // —— 列宽：D 极窄，其它做近似“自适应” —— //
    auto fit = [](int chars, int minW, int maxW)->int {
        int w = chars + 2; if (w < minW) w = minW; if (w > maxW) w = maxW; return w;
    };

    int maxLenA = 13; // JAN 13
    int maxLenC = 10;

    for (auto it = agg.cbegin(); it != agg.cend(); ++it) {
        maxLenA = std::max(maxLenA, static_cast<int>(it.key().size()));
        maxLenC = std::max(maxLenC, static_cast<int>(it.value().product.size()));
    }
    for (const auto& e : rows) {
        maxLenC = std::max(maxLenC, static_cast<int>(e.productName.size()));
    }

    xlsx.setColumnWidth(1, 1, 6);
    xlsx.setColumnWidth(2, 2, fit(maxLenA, 16, 24)); // B
    xlsx.setColumnWidth(3, 3, fit(maxLenC, 24, 60)); // C
    xlsx.setColumnWidth(4, 4, 0.5);                  // D 空白列
    xlsx.setColumnWidth(5, 5, 9);                    // E Qty
    xlsx.setColumnWidth(6, 6, 12);                   // F Unit Price / IMEI(左半)
    xlsx.setColumnWidth(7, 7, 18);                   // G Total Amount / IMEI(右半)

    if (totalAmountOut) *totalAmountOut = totalAmount;
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
    ++r; // 数据起始行 = 2

    // ---- 按 JAN 汇总：每个 JAN 只写一行，IMEI 留空 ----
    struct Agg { QString product; int qty = 0; };
    QMap<QString, Agg> agg;
    for (const auto& e : rows) {
        auto &a = agg[e.jan];
        if (a.product.isEmpty()) a.product = e.productName; // 已是“机型 容量 颜色”
        a.qty += e.qty;                                     // 逐条=1
    }

    int idx = 0; // ★ 与 Exported_Items 汇总行的索引（0-based）

    for (auto it = agg.cbegin(); it != agg.cend(); ++it, ++idx) {
        int c = 1;
        // 会員番号..到着日（6列）留空（后续可从 UI/设置填）
        for (int k = 0; k < 6; ++k) xlsx.write(r, c++, "");
        xlsx.write(r, c++, it.key());            // 7: JAN
        xlsx.write(r, c++, it.value().product);  // 8: 商品名
        xlsx.write(r, c++, "");                  // 9: IMEI（聚合留空）
        xlsx.write(r, c++, it.value().qty);      // 10: 数量

        // ★ Exported_Items 中对应的汇总行行号：
        // Exported_Items 的汇总数据是从第 4 行开始（见 writeExportedItemsSheet）
        const int exportedRow = 4 + idx;

        // ★ K 列 (11): 単価 = Exported_Items!F<exportedRow>
        const QString unitPriceRef =
            QStringLiteral("=Exported_Items!F%1").arg(exportedRow);
        xlsx.write(r, c++, unitPriceRef);

        // ★ L 列 (12): 金額 = Exported_Items!G<exportedRow>
        const QString totalAmountRef =
            QStringLiteral("=Exported_Items!G%1").arg(exportedRow);
        xlsx.write(r, c++, totalAmountRef);

        xlsx.write(r, c++, 0);                   // 13: 送料 先 0
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
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("現在のセッションに「入荷登録」の記録がなく、生成されていません。"), 2500);
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

    // 发送POST请求到API
    sendPostRequest(rows);
}

void MainWindow::openLastExport()
{
    if (m_lastExportPath.isEmpty()) {
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("まだファイルがありません。先にExcel出力してください。"), 2500);
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastExportPath));
}

void MainWindow::sendPostRequest(const QVector<ExportRow>& rows)
{
    // 获取用户输入的信息
    QString username = ui->lineEdit_8->text().trimmed();
    if (username.isEmpty()) {
        username = QStringLiteral("customer");
    }

    QString batchLevel1 = ui->lineEdit_7->text().trimmed();
    QString batchLevel2 = ui->lineEdit_9->text().trimmed();

    // 生成当前时间的ISO 8601格式字符串
    QString currentTime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    // 构建inventory_data数组
    QJsonArray inventoryData;
    for (const auto& row : rows) {
        QJsonObject item;
        item["jan"] = row.jan;
        item["imei"] = row.imei;
        inventoryData.append(item);
    }

    // 构建inventory_times对象
    QJsonObject inventoryTimes;
    inventoryTimes["actual_arrival_at"] = currentTime;

    // 构建完整的JSON请求体
    QJsonObject jsonObj;
    jsonObj["username"] = username;
    jsonObj["visit_time"] = currentTime;
    jsonObj["inventory_data"] = inventoryData;
    jsonObj["inventory_times"] = inventoryTimes;

    // batch_level_1和batch_level_2只在不为空时添加
    if (!batchLevel1.isEmpty()) {
        jsonObj["batch_level_1"] = batchLevel1;
    }
    if (!batchLevel2.isEmpty()) {
        jsonObj["batch_level_2"] = batchLevel2;
    }

    jsonObj["batch_level_3"] = m_sessionId;

    // 转换为JSON文档
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson();

    // 构建网络请求
    QNetworkRequest request(QUrl("https://data.yamaguchi.lan/api/aggregation/legal-person-offline/create-with-inventory/"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // TODO: 在此处替换为实际的Bearer Token
    request.setRawHeader("Authorization", "Bearer YOUR_BATCH_STATS_API_TOKEN");

    // 发送POST请求（不等待响应，失败时不做处理）
    m_networkManager->post(request, jsonData);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    fitAndZoom(ui->graphicsView_6);
    fitAndZoom(ui->graphicsView_7);
    fitAndZoom(ui->graphicsView_8);
    fitAndZoom(ui->graphicsView_9);
    fitAndZoom(ui->graphicsView_10);
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

    // "リセット"按钮
    if (ui->pushButton)
        connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    // tab_2: plainTextEdit 的 Enter 键检测（通过 eventFilter）
    if (ui->plainTextEdit)
        ui->plainTextEdit->installEventFilter(this);

    // tab_2: lineEdit_10/lineEdit_11 的 Enter 键检测
    if (ui->lineEdit_10)
        connect(ui->lineEdit_10, &QLineEdit::returnPressed, this, &MainWindow::onTab2JanEnter);
    if (ui->lineEdit_11)
        connect(ui->lineEdit_11, &QLineEdit::returnPressed, this, &MainWindow::onTab2ImeiEnter);
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
        box.setWindowTitle(QStringLiteral("会話選択"));
        box.setText(QStringLiteral("前回の操作（セッションID：%1）を検出しました。\n前回の操作を続行しますか？").arg(lastSid));
        QPushButton* btnContinue = box.addButton(QStringLiteral("前回の操作を続行する"), QMessageBox::AcceptRole);
        QPushButton* btnNew      = box.addButton(QStringLiteral("新しい操作を開始する"), QMessageBox::DestructiveRole);
        box.setDefaultButton(btnContinue);
        box.exec();

        if (box.clickedButton() == btnContinue) {
            m_sessionId = lastSid;
            if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("前回の操作を継続しました。"), 1500);
            return;
        }
    }
    m_sessionId = generateSessionId();
    writeLastSessionIdQSettings(m_sessionId);
    if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("新しい操作を開始しました。"), 1500);
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
        it->setData(c13, RoleCode13);
        it->setData(i15, RoleImei15);

        it->setData(baseText, RoleBaseText);
        setItemColorDot(it, hex, ui->listView_2);   // 仅圆点上色，文字保持黑色
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
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("カウンタ2はリセット済み"), 2000);
        ui->lineEdit->clear();
        return;
    }
    if (v == kCodeToSearch) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットキー：検索へ移動。"), 1500);
        return;
    }
    if (v == kCodeToTemp) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットコード：仮登録へ移動。"), 1500);
        return;
    }
    if (v == kCodeExcelExport) { ui->lineEdit->clear(); exportToExcel(); return; }
    if (v == kCodeExcelOpen)   { ui->lineEdit->clear(); openLastExport(); return; }


    // 13 位长度校验
    if (!ensureExactLenAndMark(ui->lineEdit, 13, ui->statusbar, QStringLiteral("入荷登録(JAN)"))) {
        playSound(QStringLiteral("jan_error"));  // 长度不对 → 格式错误
        return;
    }

    // ★ 新增：13 位格式正确后，立刻确认是不是已知商品 ★
    QString hex;
    const QString disp = displayNameForJan(m_db, v, &hex);
    if (disp.isEmpty()) {
        // 未知商品：不允许继续入荷登録流程
        ui->lineEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit->setFocus();
        ui->lineEdit->selectAll();
        if (ui->statusbar)
            ui->statusbar->showMessage(QStringLiteral("入荷登録：JAN未登録商品、未入庫。"), 2500);

        if (ui->label) {
            ui->label->setText(QStringLiteral("未登録 JAN"));
            ui->label->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }

        playSound(QStringLiteral("jan_not_found"));  // 这里播放“未找到商品”提示音
        return;
    }

    // 已知商品：可以进入 IMEI 输入阶段
    if (ui->label) {
        ui->label->setText(disp);
        ui->label->setStyleSheet(hex.isEmpty()
                                     ? QString()
                                     : QStringLiteral("QLabel{ color:%1; }").arg(hex));
    }

    ui->lineEdit_2->setFocus();
    ui->lineEdit_2->selectAll();
}

void MainWindow::onReg2Enter()
{
    const QString a = ui->lineEdit->text().trimmed();   // 前码 JAN
    const QString b = ui->lineEdit_2->text().trimmed(); // 后码 IMEI

    // ---- 特殊指令处理 ----
    if (b == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("カウンタ2はリセット済み"), 1500);
        return;
    }
    if (b == kCodeFlushAll) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb();
            refreshSessionRecordsView();
            showStatusOk(QStringLiteral("記録完了"));
            playSound(QStringLiteral("success"));  // 假登録批量入库成功

            m_model->clear();
            m_source = ListSource::None;
            m_lcd2Counter = 0;
            ui->lcdNumber_2->display(0);
            if (ui->label_13) ui->label_13->setText(QStringLiteral("検索結果"));
            ui->lineEdit->setFocus();
        }
        return;
    }
    if (b == kCodeToSearch) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットキー：検索へ移動。"), 1500);
        return;
    }
    if (b == kCodeToTemp) {
        ui->lineEdit->clear(); ui->lineEdit_2->clear();
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットコード：仮登録へ移動。"), 1500);
        return;
    }
    if (b == kCodeExcelExport) { ui->lineEdit->clear(); ui->lineEdit_2->clear(); exportToExcel(); return; }
    if (b == kCodeExcelOpen)   { ui->lineEdit->clear(); ui->lineEdit_2->clear(); openLastExport(); return; }


    // ---- IMEI 长度校验（必须 15 位）----
    if (!ensureExactLenAndMark(ui->lineEdit_2, 15, ui->statusbar, QStringLiteral("入荷登録(IMEI)"))) {
        playSound(QStringLiteral("imei_error"));  // IMEI 输入错误
        return;
    }

    // ---- 前码 JAN 再兜底校验（防止用户跳过前码回车）----
    if (a.size() != 13) {
        ui->lineEdit->clear();
        ui->lineEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録(JAN)に13桁を入力してください。"), 2500);
        playSound(QStringLiteral("jan_error"));   // JAN 格式错误
        return;
    }

    // ---- 业务规则 1：同会话 / 入荷登録 下 IMEI 不允许重复 ----
    if (existsInboundImeiInCurrentSession(b)) {
        ui->label->setText(QStringLiteral("IMEI重複"));
        ui->label->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        ui->lineEdit_2->clear();
        ui->lineEdit_2->setFocus(); ui->lineEdit_2->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録：IMEI 重複，未记录。"), 2000);
        playSound(QStringLiteral("imei_duplicate"));
        return;
    }

    // ---- 业务规则 2：必须是 catalog 中的已知商品，否则不入库 ----
    QString hex;
    const QString disp = displayNameForJan(m_db, a, &hex);
    if (disp.isEmpty()) {
        // 未知 JAN：格式正确但不在商品目录中 → 不执行入库
        ui->lineEdit->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar)
            ui->statusbar->showMessage(QStringLiteral("入荷登録：JAN未登録商品、未入庫。"), 2500);

        playSound(QStringLiteral("jan_not_found"));  // 播放“未找到商品”提示音

        // 可选：label 显示一下错误状态
        if (ui->label) {
            ui->label->setText(QStringLiteral("未登録 JAN"));
            ui->label->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }

        // 不写 inbound / entry_log，不更新列表、不更新计数器
        ui->lineEdit_2->clear();
        return;
    }

    // ---- 到这里说明：JAN 格式 OK + 是已知商品 + IMEI 不重复 ----
    QString err;
    const bool okInbound = insertInboundRow(QStringLiteral("入荷登録"), a, b, &err);
    const bool okLog     = insertEntryLogRow(QStringLiteral("入荷登録"), a, b);
    if (!okInbound) {
        qWarning() << "insert inbound failed:" << err;
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録：在庫登録失敗。"), 2000);
        // 失败时也不再继续后续 UI 更新
        return;
    }

    Q_UNUSED(okLog);

    // —— 列表：机型名 + 彩点 + 序号（文字黑色）—— //
    const QString left = disp; // 已知商品，直接用 disp
    const QString baseText = formatRecord(QStringLiteral("入荷登録"), {left, b});

    auto* it = new QStandardItem(baseText);
    it->setData(baseText, RoleBaseText);
    it->setData(a, RoleCode13);
    it->setData(b, RoleImei15);
    it->setData(QStringLiteral("入荷登録"), RoleKind);
    setItemColorDot(it, hex, ui->listView); // 只设置圆点

    m_model->appendRow(it);
    renumberModel(m_model);

    // 提示 + label 显示机型名（按机型色）
    showStatusOk(QStringLiteral("記録完了"));
    playSound(QStringLiteral("success"));  // 已知商品入库成功 → 成功音

    if (ui->label) {
        ui->label->setText(disp);
        ui->label->setStyleSheet(hex.isEmpty()
                                     ? QString()
                                     : QStringLiteral("QLabel{ color:%1; }").arg(hex));
    }

    // 计数器：lcdNumber_2 +1 且 /10 清零；lcdNumber 从 DB 刷新
    m_lcd2Counter = (m_lcd2Counter + 1) % 10;
    ui->lcdNumber_2->display(m_lcd2Counter);
    if (m_lcd2Counter == 0) {
        playSound(QStringLiteral("count_reset"));  // 计数器清零时播放提示音
    }
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
        auto* msg = new QStandardItem(QStringLiteral("10件完了"));
        msg->setData(QStringLiteral("10件完了"), RoleBaseText);
        m_model->appendRow(msg);
        renumberModel(m_model);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("入荷登録：10件完了、リストをリセットしました。"), 2000);
    }

    ui->lineEdit->clear();
    ui->lineEdit_2->clear();
    ui->lineEdit->setFocus();
}

// ====================== 槽：仮登録 ======================
void MainWindow::onTemp1Enter()
{
    const QString v = ui->lineEdit_4->text().trimmed();

    if (v == kCodeFlushAll) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb();
            refreshSessionRecordsView();
            showStatusOk(QStringLiteral("記録完了"));
            m_model->clear(); m_source = ListSource::None;
            m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        }
        return;
    }
    if (v == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("カウンタ2はリセット済み。"), 1500);
        ui->lineEdit_4->clear();
        return;
    }
    if (v == kCodeToSearch) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) { m_model->clear(); m_source = ListSource::None; }
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットコード：検索へ移動。"), 1500);
        return;
    }
    if (v == kCodeToRegister) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) { m_model->clear(); m_source = ListSource::None; }
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットコード：入荷登録(JAN)へ移動。"), 1500);
        return;
    }
    if (v == kCodeExcelExport) { ui->lineEdit_4->clear(); exportToExcel(); return; }
    if (v == kCodeExcelOpen)   { ui->lineEdit_4->clear(); openLastExport(); return; }


    // 13 位长度校验
    if (!ensureExactLenAndMark(ui->lineEdit_4, 13, ui->statusbar, QStringLiteral("仮登録(JAN)"))) {
        playSound(QStringLiteral("jan_error"));
        return;
    }

    // ★ 新增：立刻确认是否已知商品 ★
    QString hex;
    const QString disp = displayNameForJan(m_db, v, &hex);
    if (disp.isEmpty()) {
        ui->lineEdit_4->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit_4->setFocus();
        ui->lineEdit_4->selectAll();
        if (ui->statusbar)
            ui->statusbar->showMessage(QStringLiteral("仮登録：JAN未登録商品、未登録。"), 2500);

        if (ui->label_8) {
            ui->label_8->setText(QStringLiteral("仮登録結果: 未登録 JAN"));
            ui->label_8->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }

        playSound(QStringLiteral("jan_not_found"));
        return;
    }

    // 已知商品，才进入 IMEI 阶段
    if (ui->label_8) {
        ui->label_8->setText(QStringLiteral("仮登録結果: %1").arg(disp));
        ui->label_8->setStyleSheet(hex.isEmpty()
                                       ? QString()
                                       : QStringLiteral("QLabel{ color:%1; }").arg(hex));
    }

    ui->lineEdit_3->setFocus();
    ui->lineEdit_3->selectAll();
}

void MainWindow::onTemp2Enter()
{
    const QString a = ui->lineEdit_4->text().trimmed(); // 仮登録(前码) JAN
    const QString b = ui->lineEdit_3->text().trimmed(); // 仮登録(后码) IMEI

    // ---- 特殊指令处理 ----
    if (b == kCodeFlushAll) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb();
            refreshSessionRecordsView();
            showStatusOk(QStringLiteral("記録完了"));
            playSound(QStringLiteral("success"));  // 仮登録批量入库成功

            m_model->clear();
            m_source = ListSource::None;
            m_lcd2Counter = 0;
            ui->lcdNumber_2->display(0);
        }
        return;
    }
    if (b == kCodeResetCount) {
        m_lcd2Counter = 0; ui->lcdNumber_2->display(0);
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("カウンタ2はリセット済み。"), 1500);
        ui->lineEdit_3->clear();
        return;
    }
    if (b == kCodeToSearch) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) {
            m_model->clear(); m_source = ListSource::None;
        }
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートカットコード：仮登録へ移動。"), 1500);
        return;
    }
    if (b == kCodeToRegister) {
        ui->lineEdit_4->clear(); ui->lineEdit_3->clear();
        if (modelOnlyHasPrefix(m_model, QStringLiteral("仮登録"))) {
            m_model->clear(); m_source = ListSource::None;
        }
        ui->lineEdit->setFocus(); ui->lineEdit->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("ショートコード：入荷登録（JAN）へ移動。"), 1500);
        return;
    }
    if (b == kCodeExcelExport) { ui->lineEdit_4->clear(); ui->lineEdit_3->clear(); exportToExcel(); return; }
    if (b == kCodeExcelOpen)   { ui->lineEdit_4->clear(); ui->lineEdit_3->clear(); openLastExport(); return; }


    // ---- IMEI 长度校验（必须 15 位）----
    if (!ensureExactLenAndMark(ui->lineEdit_3, 15, ui->statusbar, QStringLiteral("仮登録(后码)"))) {
        playSound(QStringLiteral("imei_error"));
        return;
    }

    // ---- JAN 再兜底校验（防止跳过前码）----
    if (a.size() != 13) {
        ui->lineEdit_4->clear();
        ui->lineEdit_4->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("请先在 仮登録(前码) 输入 13 位。"), 2500);
        playSound(QStringLiteral("jan_error"));
        return;
    }

    // ---- 已入荷登记 IMEI 不能重复 ----
    const bool dupNow = existsInboundImeiInCurrentSession(b);

    // ---- 业务规则：仮登録 也必须是已知商品 ----
    QString hex;
    const QString disp = displayNameForJan(m_db, a, &hex);
    if (disp.isEmpty()) {
        // 未知 JAN：不加入仮登録列表
        ui->lineEdit_4->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit_4->setFocus(); ui->lineEdit_4->selectAll();

        if (ui->statusbar)
            ui->statusbar->showMessage(QStringLiteral("仮登録：JAN 未登録商品，未加入。"), 2500);

        playSound(QStringLiteral("jan_not_found"));

        if (ui->label_8) {
            ui->label_8->setText(QStringLiteral("仮登録結果: 未登録 JAN"));
            ui->label_8->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }

        ui->lineEdit_3->clear();
        return;
    }

    // 到这里：JAN 格式正确 + catalog 中已存在
    const QString left = disp;
    const QString baseText = formatRecord(QStringLiteral("仮登録"), {left, b});

    if (dupNow) {
        // 重复：标红 + 彩点，仅用于提醒，不会入库
        addToListWithSource(baseText, ListSource::Temp, QColor("#d32f2f"));
        if (m_model->rowCount() > 0) {
            auto* last = m_model->item(m_model->rowCount()-1);
            if (last) {
                last->setData(baseText, RoleBaseText);
                last->setData(a, RoleCode13);
                last->setData(b, RoleImei15);
                last->setData(QStringLiteral("仮登録"), RoleKind);
                setItemColorDot(last, hex, ui->listView);
            }
        }
        if (ui->statusbar)
            ui->statusbar->showMessage(QStringLiteral("仮登録：IMEI 重复，已标红（不会入库）。"), 2000);
        playSound(QStringLiteral("imei_duplicate"));
        if (ui->label_8) {
            ui->label_8->setText(QStringLiteral("仮登録結果: %1（重複）").arg(left));
            ui->label_8->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }
    } else {
        // 正常仮登録：黑字 + 彩点
        addToListWithSource(baseText, ListSource::Temp /* no color */);
        if (m_model->rowCount() > 0) {
            auto* last = m_model->item(m_model->rowCount()-1);
            if (last) {
                last->setData(baseText, RoleBaseText);
                last->setData(a, RoleCode13);
                last->setData(b, RoleImei15);
                last->setData(QStringLiteral("仮登録"), RoleKind);
                setItemColorDot(last, hex, ui->listView);
            }
        }
        if (ui->statusbar)
            ui->statusbar->showMessage(QStringLiteral("仮登録：已加入。"), 1500);
        if (ui->label_8) {
            ui->label_8->setText(QStringLiteral("仮登録結果: %1").arg(left));
            ui->label_8->setStyleSheet(hex.isEmpty()
                                           ? QString()
                                           : QStringLiteral("QLabel{ color:%1; }").arg(hex));
        }
    }

    renumberModel(m_model);
    ui->lineEdit_4->clear();
    ui->lineEdit_3->clear();
    ui->lineEdit_4->setFocus();
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
    if (v == kCodeExcelExport) { ui->lineEdit_6->clear(); exportToExcel(); return; }
    if (v == kCodeExcelOpen)   { ui->lineEdit_6->clear(); openLastExport(); return; }


    if (!ensureExactLenAndMark(ui->lineEdit_6, 13, ui->statusbar, QStringLiteral("検索(前码)"))) {
        playSound(QStringLiteral("jan_error"));  // 13 位不对 → JAN 格式错误
        return;
    }

    ui->lineEdit_5->setFocus();
    ui->lineEdit_5->selectAll();
}

void MainWindow::onSearch2Enter()
{
    const QString a = ui->lineEdit_6->text().trimmed();
    const QString b = ui->lineEdit_5->text().trimmed();

    if (b == kCodeFlushAll) {
        ui->lineEdit_6->clear(); ui->lineEdit_5->clear();
        if (hasTempInListView() && flushAllListItemsToDb()) {
            updateLcdFromDb(); refreshSessionRecordsView(); showStatusOk(QStringLiteral("記録完了"));
            playSound("success");  // 播放假登录批量入库成功提示音
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
    if (b == kCodeExcelExport) { ui->lineEdit_6->clear(); ui->lineEdit_5->clear(); exportToExcel(); return; }
    if (b == kCodeExcelOpen)   { ui->lineEdit_6->clear(); ui->lineEdit_5->clear(); openLastExport(); return; }


    if (!ensureExactLenAndMark(ui->lineEdit_5, 15, ui->statusbar, QStringLiteral("検索(后码)"))) {
        playSound("imei_error");  // 播放 IMEI 输入错误提示音
        return;
    }

    if (a.size() != 13) {
        ui->lineEdit_6->clear();
        ui->lineEdit_6->setStyleSheet("QLineEdit { background-color: #ffcccc; }");
        ui->lineEdit_6->setFocus(); ui->lineEdit_6->selectAll();
        if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("请先在 検索(前码) 输入 13 位。"), 2500);
        playSound("jan_error");  // 播放 JAN code 输入错误提示音
        return;
    }

    // 业务：検索显示（机型名 + 彩点 + 序号）
    // === 検索表现 ===
    QString hex;
    const QString disp = displayNameForJan(m_db, a, &hex);

    // 在右侧会话列表中查找是否已入库（按 IMEI 匹配）
    int hitRow = -1;
    for (int r = 0; r < m_modelSession->rowCount(); ++r) {
        if (auto *it = m_modelSession->item(r)) {
            if (it->data(RoleImei15).toString() == b) { hitRow = r; break; }
        }
    }

    if (hitRow >= 0) {
        // 命中：右侧第 hitRow 行记入済み
        auto *it = m_modelSession->item(hitRow);
        if (it) {
            it->setForeground(QBrush(QColor("#0a7f3f")));           // 绿色
            ui->listView_2->setCurrentIndex(m_modelSession->index(hitRow, 0));
            ui->listView_2->scrollTo(m_modelSession->index(hitRow, 0));
        }

        if (ui->label_13) {
            const int no = hitRow + 1;
            const QString left = disp.isEmpty() ? a : disp;
            ui->label_13->setText(QStringLiteral("検索結果: %1 / No.%2")
                                      .arg(left)
                                      .arg(no, 2, 10, QChar('0')));
            ui->label_13->setStyleSheet(QString()); // 绿色留给右侧行
        }

    } else {
        // 未命中：本会话未登记
        if (ui->label_13) {
            ui->label_13->setText(QStringLiteral("今回では未登録"));
            ui->label_13->setStyleSheet(QStringLiteral("QLabel{ color:#d32f2f; font-weight:600; }"));
        }
    }

    // 左侧“検索”列表照常追加（可选保留）
    const QString left = disp.isEmpty() ? a : disp;
    const QString baseText = formatRecord(QStringLiteral("検索"), {left, b});
    addToListWithSource(baseText, ListSource::Search);
    if (m_model->rowCount() > 0) {
        if (auto* last = m_model->item(m_model->rowCount()-1)) {
            last->setData(baseText, RoleBaseText);
            last->setData(a, RoleCode13);
            last->setData(b, RoleImei15);
            last->setData(QStringLiteral("検索"), RoleKind);
            setItemColorDot(last, hex, ui->listView);
        }
    }
    renumberModel(m_model);

    if (ui->statusbar) ui->statusbar->showMessage(QStringLiteral("検索：已更新列表。"), 1500);

    ui->lineEdit_6->clear();
    ui->lineEdit_5->clear();
    ui->lineEdit_6->setFocus();

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

// ====================== tab_2: 解析V4格式 ======================
QVector<MainWindow::ParsedRecord> MainWindow::parseV4Line(const QString& line, QStringList* errors)
{
    QVector<ParsedRecord> results;

    // 支持全角逗号和半角逗号
    QString normalized = line;
    normalized.replace(QStringLiteral("，"), QStringLiteral(","));
    const QStringList parts = normalized.split(QStringLiteral(","), Qt::SkipEmptyParts);

    QString jan;
    QString imei;

    for (const QString& part : parts) {
        const QString p = part.trimmed();

        // 提取GTIN (JAN)
        if (p.startsWith(QStringLiteral("GTIN"), Qt::CaseInsensitive)) {
            QString gtinValue = p.mid(4);  // 去掉"GTIN"前缀
            // 去掉前导0
            while (gtinValue.startsWith('0') && gtinValue.size() > 13) {
                gtinValue = gtinValue.mid(1);
            }
            if (gtinValue.size() == 13) {
                jan = gtinValue;
            } else {
                if (errors) errors->append(QStringLiteral("JANコードは13桁である必要があります（現在：%1桁）").arg(gtinValue.size()));
            }
        }

        // 提取IMEI
        if (p.startsWith(QStringLiteral("IMEI"), Qt::CaseInsensitive)) {
            QString imeiValue = p.mid(4);  // 去掉"IMEI"前缀
            if (imeiValue.size() == 15) {
                imei = imeiValue;
            } else {
                if (errors) errors->append(QStringLiteral("IMEIは15桁である必要があります（現在：%1桁）").arg(imeiValue.size()));
            }
        }
    }

    // 如果JAN和IMEI都有效，则创建记录
    if (!jan.isEmpty() && !imei.isEmpty()) {
        ParsedRecord rec;
        rec.jan = jan;
        rec.imei = imei;
        rec.valid = true;
        results.append(rec);
    }

    return results;
}

// ====================== tab_2: 解析V3格式 ======================
QVector<MainWindow::ParsedRecord> MainWindow::parseV3Line(const QString& line, QStringList* errors)
{
    QVector<ParsedRecord> results;

    // 支持全角逗号和半角逗号
    QString normalized = line;
    normalized.replace(QStringLiteral("，"), QStringLiteral(","));
    const QStringList parts = normalized.split(QStringLiteral(","), Qt::SkipEmptyParts);

    QString jan;
    QStringList serials;

    for (const QString& part : parts) {
        const QString p = part.trimmed();

        // 提取GTIN (JAN)
        if (p.startsWith(QStringLiteral("GTIN"), Qt::CaseInsensitive)) {
            QString gtinValue = p.mid(4);  // 去掉"GTIN"前缀
            // 去掉前导0
            while (gtinValue.startsWith('0') && gtinValue.size() > 13) {
                gtinValue = gtinValue.mid(1);
            }
            if (gtinValue.size() == 13) {
                jan = gtinValue;
            } else {
                if (errors) errors->append(QStringLiteral("JANコードは13桁である必要があります（現在：%1桁）").arg(gtinValue.size()));
            }
        }

        // 提取序列号（以S开头，保留完整，11位）
        if (p.startsWith('S') && p.size() == 11) {
            // 验证是否全是字母数字
            bool valid = true;
            for (const QChar& c : p) {
                if (!c.isLetterOrNumber()) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                serials.append(p);
            } else {
                if (errors) errors->append(QStringLiteral("シリアル番号の形式が無効です：%1").arg(p));
            }
        } else if (p.startsWith('S') && p.size() != 11 && !p.startsWith(QStringLiteral("SSCC"), Qt::CaseInsensitive) && !p.startsWith(QStringLiteral("SCC"), Qt::CaseInsensitive)) {
            // 以S开头但不是11位，且不是SSCC/SCC字段
            if (errors) errors->append(QStringLiteral("シリアル番号は11桁である必要があります（現在：%1桁）：%2").arg(p.size()).arg(p));
        }
    }

    // 为每个有效的序列号创建记录
    if (!jan.isEmpty()) {
        for (const QString& serial : serials) {
            ParsedRecord rec;
            rec.jan = jan;
            rec.imei = serial;
            rec.valid = true;
            results.append(rec);
        }
    }

    return results;
}

// ====================== tab_2: 显示错误信息 ======================
void MainWindow::showTab2Error(const QString& text)
{
    if (ui->label_3) {
        ui->label_3->setText(text);
        ui->label_3->setStyleSheet(QStringLiteral("QLabel{ color:#cc0000; }"));  // 红色
    }
    if (ui->statusbar) {
        ui->statusbar->setStyleSheet(QStringLiteral("QStatusBar{ color:#cc0000; }"));  // 红色
        ui->statusbar->showMessage(text, 3000);
        QTimer::singleShot(3000, this, [this]{
            ui->statusbar->setStyleSheet(m_statusDefaultStyle);
        });
    }
}

// ====================== tab_2: 刷新listView_3 ======================
void MainWindow::refreshTab2ListView()
{
    if (!m_modelTab2) return;
    m_modelTab2->clear();

    QSqlQuery q(m_db);
    q.prepare("SELECT code13, imei15 FROM inbound "
              "WHERE session_id=? AND kind='入荷登録' ORDER BY id ASC");
    q.addBindValue(m_sessionId);

    if (!q.exec()) {
        qWarning() << "refreshTab2ListView failed:" << q.lastError();
        return;
    }

    int seq = 1;
    while (q.next()) {
        const QString jan = q.value(0).toString();
        const QString imei = q.value(1).toString();

        // 查询商品名称（先查 Other Products 映射表，再查 catalog 数据库）
        QString productName = productNameForJan(jan);
        if (productName.isEmpty()) {
            productName = displayNameForJan(m_db, jan);
        }

        QString displayText;
        if (productName.isEmpty()) {
            displayText = QStringLiteral("%1. %2 | %3")
                              .arg(seq, 2, 10, QChar('0'))
                              .arg(jan)
                              .arg(imei);
        } else {
            displayText = QStringLiteral("%1. %2 | %3 | %4")
                              .arg(seq, 2, 10, QChar('0'))
                              .arg(jan)
                              .arg(imei)
                              .arg(productName);
        }

        auto* item = new QStandardItem(displayText);
        m_modelTab2->appendRow(item);
        ++seq;
    }

    m_tab2Counter = seq - 1;
    ui->lcdNumber_3->display(m_tab2Counter);
}

// ====================== tab_2: plainTextEdit Enter处理 ======================
void MainWindow::onPlainTextEnter()
{
    const QString text = ui->plainTextEdit->toPlainText().trimmed();
    if (text.isEmpty()) {
        ui->plainTextEdit->clear();
        return;
    }

    QStringList errors;
    QVector<ParsedRecord> records;

    // 判断数据类型并解析
    if (text.startsWith(QStringLiteral("V4"), Qt::CaseInsensitive)) {
        records = parseV4Line(text, &errors);
    } else if (text.startsWith(QStringLiteral("V3"), Qt::CaseInsensitive)) {
        records = parseV3Line(text, &errors);
    } else {
        showTab2Error(QStringLiteral("未知のデータ形式です。V3またはV4で始まる必要があります。"));
        playSound(QStringLiteral("error"));
        ui->plainTextEdit->clear();
        return;
    }

    // 处理解析结果
    int successCount = 0;
    int duplicateCount = 0;
    QStringList duplicateImeis;

    for (const ParsedRecord& rec : records) {
        if (!rec.valid) continue;

        // 检查重复
        if (existsInboundImeiInCurrentSession(rec.imei)) {
            ++duplicateCount;
            duplicateImeis.append(rec.imei);
            continue;
        }

        // 写入数据库
        QString errText;
        if (insertInboundRow(QStringLiteral("入荷登録"), rec.jan, rec.imei, &errText)) {
            ++successCount;
        } else {
            errors.append(errText);
        }
    }

    // 更新UI
    ui->plainTextEdit->clear();

    // 刷新listView_3
    refreshTab2ListView();

    // 更新右侧会话记录（如果需要）
    refreshSessionRecordsView();

    // 更新lcdNumber（主计数器）
    updateLcdFromDb();

    // 显示结果
    if (successCount > 0) {
        // 清除之前的错误样式
        if (ui->label_3) {
            ui->label_3->setText(QStringLiteral("ログ"));
            ui->label_3->setStyleSheet(QString());
        }

        showStatusOk(QStringLiteral("登録完了：%1件").arg(successCount));
        playSound(QStringLiteral("success"));
    }

    // 显示错误（如果有）
    if (!errors.isEmpty() || duplicateCount > 0) {
        QStringList allErrors = errors;
        if (duplicateCount > 0) {
            allErrors.prepend(QStringLiteral("重複：このIMEI/シリアル番号は既に登録されています（%1件）").arg(duplicateCount));
        }

        const QString errorMsg = allErrors.join(QStringLiteral("; "));
        if (ui->label_3) {
            ui->label_3->setText(errorMsg);
            ui->label_3->setStyleSheet(QStringLiteral("QLabel{ color:#cc0000; }"));
        }

        // 如果没有成功记录，播放错误音
        if (successCount == 0) {
            if (ui->statusbar) {
                ui->statusbar->setStyleSheet(QStringLiteral("QStatusBar{ color:#cc0000; }"));
                ui->statusbar->showMessage(errorMsg, 3000);
                QTimer::singleShot(3000, this, [this]{
                    ui->statusbar->setStyleSheet(m_statusDefaultStyle);
                });
            }
            playSound(QStringLiteral("error"));
        }
    }
}

// ====================== tab_2: label_2 日志显示 ======================
void MainWindow::showTab2Label2Log(const QString& text, bool isError)
{
    if (!ui->label_2) return;

    if (isError) {
        ui->label_2->setStyleSheet(QStringLiteral("QLabel{ color:#cc0000; }"));  // 红色
    } else {
        ui->label_2->setStyleSheet(QStringLiteral("QLabel{ color:#0a6d2a; }"));  // 绿色
    }
    ui->label_2->setText(text);
    // 信息保持显示，直到有新的信息替换
}

// ====================== tab_2: lineEdit_10 JAN 输入处理 ======================
void MainWindow::onTab2JanEnter()
{
    const QString jan = ui->lineEdit_10->text().trimmed();

    // 验证：必须是13位数字
    if (jan.size() != 13) {
        showTab2Label2Log(QStringLiteral("JANコードは13桁である必要があります（現在：%1桁）").arg(jan.size()), true);
        playSound(QStringLiteral("jan_error"));
        ui->lineEdit_10->clear();
        ui->lineEdit_10->setFocus();
        return;
    }

    // 验证：必须全是数字
    bool allDigits = true;
    for (const QChar& c : jan) {
        if (!c.isDigit()) {
            allDigits = false;
            break;
        }
    }
    if (!allDigits) {
        showTab2Label2Log(QStringLiteral("JANコードは数字のみである必要があります"), true);
        playSound(QStringLiteral("jan_error"));
        ui->lineEdit_10->clear();
        ui->lineEdit_10->setFocus();
        return;
    }

    // 验证：必须在硬编码数据或catalog数据库中找到对应商品
    QString productName = productNameForJan(jan);
    if (productName.isEmpty()) {
        productName = displayNameForJan(m_db, jan);
    }

    if (productName.isEmpty()) {
        showTab2Label2Log(QStringLiteral("該当する商品が見つかりません：%1").arg(jan), true);
        playSound(QStringLiteral("jan_not_found"));
        ui->lineEdit_10->clear();
        ui->lineEdit_10->setFocus();
        return;
    }

    // 验证通过：保存JAN，显示商品信息，跳转焦点
    m_tab2PendingJan = jan;

    // 在label_16显示商品信息（如果存在）
    if (ui->label_16) {
        ui->label_16->setText(productName);
    }

    showTab2Label2Log(QStringLiteral("商品確認：%1").arg(productName), false);
    ui->lineEdit_11->setFocus();
}

// ====================== tab_2: lineEdit_11 IMEI/序列号 输入处理 ======================
void MainWindow::onTab2ImeiEnter()
{
    const QString imei = ui->lineEdit_11->text().trimmed();

    // 验证：必须是15位（IMEI）或11位（序列号）
    if (imei.size() != 15 && imei.size() != 11) {
        showTab2Label2Log(QStringLiteral("IMEI（15桁）またはシリアル番号（11桁）である必要があります（現在：%1桁）").arg(imei.size()), true);
        playSound(QStringLiteral("imei_error"));
        ui->lineEdit_11->clear();
        ui->lineEdit_11->setFocus();
        return;
    }

    // 如果是15位，验证必须全是数字（IMEI）
    if (imei.size() == 15) {
        bool allDigits = true;
        for (const QChar& c : imei) {
            if (!c.isDigit()) {
                allDigits = false;
                break;
            }
        }
        if (!allDigits) {
            showTab2Label2Log(QStringLiteral("IMEIは数字のみである必要があります"), true);
            playSound(QStringLiteral("imei_error"));
            ui->lineEdit_11->clear();
            ui->lineEdit_11->setFocus();
            return;
        }
    }

    // 如果是11位，验证必须全是字母数字（序列号）
    if (imei.size() == 11) {
        bool allAlphaNum = true;
        for (const QChar& c : imei) {
            if (!c.isLetterOrNumber()) {
                allAlphaNum = false;
                break;
            }
        }
        if (!allAlphaNum) {
            showTab2Label2Log(QStringLiteral("シリアル番号は英数字のみである必要があります"), true);
            playSound(QStringLiteral("imei_error"));
            ui->lineEdit_11->clear();
            ui->lineEdit_11->setFocus();
            return;
        }
    }

    // 检查是否有待处理的JAN
    if (m_tab2PendingJan.isEmpty()) {
        showTab2Label2Log(QStringLiteral("先にJANコードを入力してください"), true);
        playSound(QStringLiteral("jan_error"));
        ui->lineEdit_10->setFocus();
        return;
    }

    // 重复检测
    if (existsInboundImeiInCurrentSession(imei)) {
        showTab2Label2Log(QStringLiteral("重複：このIMEI/シリアル番号は既に登録されています"), true);
        playSound(QStringLiteral("imei_duplicate"));
        ui->lineEdit_11->clear();
        ui->lineEdit_11->setFocus();
        return;
    }

    // 写入数据库
    QString errText;
    if (!insertInboundRow(QStringLiteral("入荷登録"), m_tab2PendingJan, imei, &errText)) {
        showTab2Label2Log(QStringLiteral("登録失敗：%1").arg(errText), true);
        playSound(QStringLiteral("error"));
        ui->lineEdit_11->clear();
        ui->lineEdit_11->setFocus();
        return;
    }

    // 写入成功
    showTab2Label2Log(QStringLiteral("登録完了：1件"), false);
    showStatusOk(QStringLiteral("登録完了：1件"));
    playSound(QStringLiteral("success"));

    // 更新UI
    refreshTab2ListView();
    refreshSessionRecordsView();
    updateLcdFromDb();

    // 清空输入框，重置状态
    ui->lineEdit_10->clear();
    ui->lineEdit_11->clear();
    m_tab2PendingJan.clear();

    // 清空商品信息显示
    if (ui->label_16) {
        ui->label_16->setText(QStringLiteral("商品情報"));
    }

    // 焦点跳回lineEdit_10
    ui->lineEdit_10->setFocus();
}
