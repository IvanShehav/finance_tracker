#include "MainWindow.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMath>

namespace {

QDoubleSpinBox* moneySpinBox(double maxValue)
{
    auto* spin = new QDoubleSpinBox;
    spin->setRange(0.0, maxValue);
    spin->setDecimals(2);
    spin->setPrefix("RUB ");
    spin->setSingleStep(100.0);
    spin->setMinimumHeight(42);
    return spin;
}

QDateEdit* calendarEdit(const QDate& date)
{
    auto* edit = new QDateEdit(date);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat("dd.MM.yyyy");
    edit->setMinimumHeight(42);
    edit->setDateRange(QDate(2020, 1, 1), QDate::currentDate().addYears(2));
    return edit;
}

QTableWidgetItem* tableItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

QFrame* makeCard()
{
    auto* card = new QFrame;
    card->setObjectName("card");
    return card;
}

QLabel* metricLabel()
{
    auto* label = new QLabel;
    label->setObjectName("metricMain");
    label->setMinimumHeight(34);
    return label;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_manager.load();

    setWindowTitle(QStringLiteral("Finance Tracker"));
    resize(1240, 780);

    connect(&m_manager, &FinanceManager::changed, this, &MainWindow::refreshViews);
    connect(&m_insiderService, &InsiderService::tradesLoaded, this, &MainWindow::showInsiderTrades);
    connect(&m_marketService, &MarketService::marketDataLoaded, this, &MainWindow::showMarketData);

    rebuildUi();
}

void MainWindow::rebuildUi()
{
    m_quizGroups.clear();
    m_quizQuestionLabels.clear();
    m_quizButtons.clear();
    m_courses = createCourses();

    auto* shell = new QWidget;
    auto* shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(18, 18, 18, 18);
    shellLayout->setSpacing(12);

    auto* topCard = makeCard();
    auto* topLayout = new QHBoxLayout(topCard);
    auto* appTitle = new QLabel(QStringLiteral("Finance Tracker"));
    appTitle->setObjectName("appTitle");

    m_themeButton = new QPushButton(m_darkMode ? t(QStringLiteral("Светлая"), QStringLiteral("Light"))
                                               : t(QStringLiteral("Темная"), QStringLiteral("Dark")));
    m_themeButton->setObjectName("topToggleButton");
    m_themeButton->setFixedSize(92, 34);

    m_languageButton = new QPushButton(m_language == UiLanguage::English ? QStringLiteral("EN") : QStringLiteral("RU"));
    m_languageButton->setObjectName("topToggleButton");
    m_languageButton->setFixedSize(92, 34);

    topLayout->addWidget(appTitle, 1);
    topLayout->addWidget(m_themeButton);
    topLayout->addWidget(m_languageButton);
    shellLayout->addWidget(topCard);

    m_tabs = new QTabWidget;
    m_tabs->setDocumentMode(true);
    m_tabs->addTab(buildTodayTab(), t(QStringLiteral("Сегодня"), QStringLiteral("Today")));
    m_tabs->addTab(buildReportsTab(), t(QStringLiteral("Отчеты"), QStringLiteral("Reports")));
    m_tabs->addTab(buildAssetsTab(), t(QStringLiteral("Активы"), QStringLiteral("Assets")));
    m_tabs->addTab(buildMarketsTab(), t(QStringLiteral("Рынки"), QStringLiteral("Markets")));
    m_tabs->addTab(buildInsiderTab(), t(QStringLiteral("Инсайдеры"), QStringLiteral("Insiders")));
    m_tabs->addTab(buildEducationTab(), t(QStringLiteral("Обучение"), QStringLiteral("Learn")));
    shellLayout->addWidget(m_tabs, 1);

    setCentralWidget(shell);

    connect(m_themeButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    connect(m_languageButton, &QPushButton::clicked, this, &MainWindow::toggleLanguage);

    applyTheme();
    refreshViews();
    renderCourse(qBound(0, m_currentCourseIndex, m_courses.size() - 1));
}

QWidget* MainWindow::buildTodayTab()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(18);

    auto* title = new QLabel(t(QStringLiteral("Быстрый учет расходов"), QStringLiteral("Quick expense entry")));
    title->setObjectName("pageTitle");
    root->addWidget(title);

    auto* metrics = new QGridLayout;
    metrics->setSpacing(14);

    auto* spentCard = makeCard();
    auto* spentLayout = new QVBoxLayout(spentCard);
    spentLayout->addWidget(new QLabel(t(QStringLiteral("Потрачено сегодня"), QStringLiteral("Spent today"))));
    m_todaySpentLabel = metricLabel();
    spentLayout->addWidget(m_todaySpentLabel);

    auto* limitCard = makeCard();
    auto* limitLayout = new QVBoxLayout(limitCard);
    limitLayout->addWidget(new QLabel(t(QStringLiteral("Дневной лимит"), QStringLiteral("Daily limit"))));
    m_todayLimitLabel = metricLabel();
    limitLayout->addWidget(m_todayLimitLabel);

    auto* remainingCard = makeCard();
    auto* remainingLayout = new QVBoxLayout(remainingCard);
    remainingLayout->addWidget(new QLabel(t(QStringLiteral("Можно еще потратить"), QStringLiteral("Available to spend"))));
    m_todayRemainingLabel = metricLabel();
    remainingLayout->addWidget(m_todayRemainingLabel);

    metrics->addWidget(spentCard, 0, 0);
    metrics->addWidget(limitCard, 0, 1);
    metrics->addWidget(remainingCard, 0, 2);
    root->addLayout(metrics);

    auto* editorCard = makeCard();
    auto* editorLayout = new QGridLayout(editorCard);
    editorLayout->setHorizontalSpacing(12);
    editorLayout->setVerticalSpacing(12);

    m_dailyLimitSpin = moneySpinBox(1000000.0);
    auto* saveLimitButton = new QPushButton(t(QStringLiteral("Сохранить лимит"), QStringLiteral("Save limit")));
    saveLimitButton->setObjectName("primaryButton");

    m_expenseTitleEdit = new QLineEdit;
    m_expenseTitleEdit->setPlaceholderText(t(QStringLiteral("Что купил: кофе, обед, метро"), QStringLiteral("What you bought: coffee, lunch, subway")));
    m_expenseTitleEdit->setMinimumHeight(42);

    m_expenseDateEdit = calendarEdit(QDate::currentDate());

    m_expenseCategoryPanel = new QWidget;
    auto* categoryLayout = new QHBoxLayout(m_expenseCategoryPanel);
    categoryLayout->setContentsMargins(0, 0, 0, 0);
    categoryLayout->setSpacing(8);
    m_expenseCategoryGroup = new QButtonGroup(m_expenseCategoryPanel);
    m_expenseCategoryGroup->setExclusive(true);
    for (const CategoryBudget& budget : m_manager.budgets()) {
        auto* button = new QPushButton(displayCategory(budget.category));
        button->setCheckable(true);
        button->setObjectName("chipButton");
        button->setProperty("category", budget.category);
        m_expenseCategoryGroup->addButton(button);
        categoryLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, button]() {
            m_selectedExpenseCategory = button->property("category").toString();
        });
        if (m_selectedExpenseCategory.isEmpty()) {
            m_selectedExpenseCategory = budget.category;
            button->setChecked(true);
        }
    }
    categoryLayout->addStretch();

    m_expenseAmountSpin = moneySpinBox(1000000.0);
    m_expenseAmountSpin->setMinimum(0.01);

    m_addExpenseButton = new QPushButton(t(QStringLiteral("Добавить трату"), QStringLiteral("Add expense")));
    m_addExpenseButton->setObjectName("primaryButton");
    m_cancelExpenseEditButton = new QPushButton(t(QStringLiteral("Отмена"), QStringLiteral("Cancel")));
    m_cancelExpenseEditButton->setObjectName("toolbarButton");
    m_cancelExpenseEditButton->hide();

    editorLayout->addWidget(new QLabel(t(QStringLiteral("Лимит на день"), QStringLiteral("Limit per day"))), 0, 0);
    editorLayout->addWidget(m_dailyLimitSpin, 0, 1);
    editorLayout->addWidget(saveLimitButton, 0, 2);
    editorLayout->addWidget(new QLabel(t(QStringLiteral("Новая трата"), QStringLiteral("New expense"))), 1, 0);
    editorLayout->addWidget(m_expenseTitleEdit, 1, 1);
    editorLayout->addWidget(m_expenseDateEdit, 1, 2);
    editorLayout->addWidget(m_expenseAmountSpin, 1, 3);
    editorLayout->addWidget(m_addExpenseButton, 1, 4);
    editorLayout->addWidget(m_cancelExpenseEditButton, 2, 4);
    editorLayout->addWidget(new QLabel(t(QStringLiteral("Категория"), QStringLiteral("Category"))), 2, 0);
    editorLayout->addWidget(m_expenseCategoryPanel, 2, 1, 1, 4);
    root->addWidget(editorCard);

    m_todayStatusLabel = new QLabel;
    m_todayStatusLabel->setObjectName("statusLabel");
    root->addWidget(m_todayStatusLabel);

    m_todayExpenseTable = new QTableWidget;
    m_todayExpenseTable->setColumnCount(4);
    m_todayExpenseTable->setHorizontalHeaderLabels({
        t(QStringLiteral("Дата"), QStringLiteral("Date")),
        t(QStringLiteral("Категория"), QStringLiteral("Category")),
        t(QStringLiteral("Описание"), QStringLiteral("Description")),
        t(QStringLiteral("Сумма"), QStringLiteral("Amount"))
    });
    m_todayExpenseTable->horizontalHeader()->setStretchLastSection(true);
    m_todayExpenseTable->verticalHeader()->setVisible(false);
    m_todayExpenseTable->setAlternatingRowColors(true);
    m_todayExpenseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_todayExpenseTable->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_todayExpenseTable, 1);

    auto* expenseActions = new QHBoxLayout;
    m_editExpenseButton = new QPushButton(t(QStringLiteral("Редактировать выбранную"), QStringLiteral("Edit selected")));
    m_editExpenseButton->setObjectName("toolbarButton");
    m_deleteExpenseButton = new QPushButton(t(QStringLiteral("Удалить выбранную"), QStringLiteral("Delete selected")));
    m_deleteExpenseButton->setObjectName("toolbarButton");
    expenseActions->addStretch();
    expenseActions->addWidget(m_editExpenseButton);
    expenseActions->addWidget(m_deleteExpenseButton);
    root->addLayout(expenseActions);

    connect(saveLimitButton, &QPushButton::clicked, this, &MainWindow::saveDailyLimit);
    connect(m_addExpenseButton, &QPushButton::clicked, this, &MainWindow::addExpense);
    connect(m_cancelExpenseEditButton, &QPushButton::clicked, this, &MainWindow::cancelExpenseEdit);
    connect(m_editExpenseButton, &QPushButton::clicked, this, &MainWindow::editSelectedExpense);
    connect(m_deleteExpenseButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedExpense);

    return page;
}

QWidget* MainWindow::buildReportsTab()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* top = new QHBoxLayout;
    auto* title = new QLabel(t(QStringLiteral("Отчеты по тратам"), QStringLiteral("Expense reports")));
    title->setObjectName("pageTitle");
    m_reportFromDateEdit = calendarEdit(QDate::currentDate().addMonths(-1));
    m_reportToDateEdit = calendarEdit(QDate::currentDate());
    m_reportFromDateEdit->installEventFilter(this);
    m_reportToDateEdit->installEventFilter(this);
    top->addWidget(title, 1);
    top->addWidget(new QLabel(t(QStringLiteral("С"), QStringLiteral("From"))));
    top->addWidget(m_reportFromDateEdit);
    top->addWidget(new QLabel(t(QStringLiteral("По"), QStringLiteral("To"))));
    top->addWidget(m_reportToDateEdit);
    root->addLayout(top);

    m_reportSummaryLabel = new QLabel;
    m_reportSummaryLabel->setObjectName("statusLabel");
    root->addWidget(m_reportSummaryLabel);

    auto* splitter = new QSplitter(Qt::Horizontal);
    m_reportTable = new QTableWidget;
    m_reportTable->setColumnCount(3);
    m_reportTable->setHorizontalHeaderLabels({
        t(QStringLiteral("Категория"), QStringLiteral("Category")),
        t(QStringLiteral("Потрачено"), QStringLiteral("Spent")),
        t(QStringLiteral("Доля"), QStringLiteral("Share"))
    });
    m_reportTable->horizontalHeader()->setStretchLastSection(true);
    m_reportTable->verticalHeader()->setVisible(false);
    m_reportTable->setAlternatingRowColors(true);

    m_budgetChart = new BudgetChartWidget;
    m_budgetChart->setTexts(
        t(QStringLiteral("Расходы по категориям"), QStringLiteral("Expenses by category")),
        t(QStringLiteral("Пока нет расходов за выбранный период"), QStringLiteral("No expenses for the selected period"))
    );
    splitter->addWidget(m_reportTable);
    splitter->addWidget(m_budgetChart);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    root->addWidget(splitter, 1);

    auto* historyTitle = new QLabel(t(QStringLiteral("История трат за период"), QStringLiteral("Expense history for period")));
    historyTitle->setObjectName("sectionTitle");
    root->addWidget(historyTitle);

    m_reportExpenseTable = new QTableWidget;
    m_reportExpenseTable->setColumnCount(4);
    m_reportExpenseTable->setHorizontalHeaderLabels({
        t(QStringLiteral("Дата"), QStringLiteral("Date")),
        t(QStringLiteral("Категория"), QStringLiteral("Category")),
        t(QStringLiteral("Описание"), QStringLiteral("Description")),
        t(QStringLiteral("Сумма"), QStringLiteral("Amount"))
    });
    m_reportExpenseTable->horizontalHeader()->setStretchLastSection(true);
    m_reportExpenseTable->verticalHeader()->setVisible(false);
    m_reportExpenseTable->setAlternatingRowColors(true);
    m_reportExpenseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_reportExpenseTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_reportExpenseTable->setMaximumHeight(190);
    root->addWidget(m_reportExpenseTable);

    auto* historyActions = new QHBoxLayout;
    auto* editHistoryButton = new QPushButton(t(QStringLiteral("Редактировать выбранную"), QStringLiteral("Edit selected")));
    editHistoryButton->setObjectName("toolbarButton");
    auto* deleteHistoryButton = new QPushButton(t(QStringLiteral("Удалить выбранную"), QStringLiteral("Delete selected")));
    deleteHistoryButton->setObjectName("toolbarButton");
    historyActions->addStretch();
    historyActions->addWidget(editHistoryButton);
    historyActions->addWidget(deleteHistoryButton);
    root->addLayout(historyActions);

    connect(m_reportFromDateEdit, &QDateEdit::dateChanged, this, &MainWindow::refreshViews);
    connect(m_reportToDateEdit, &QDateEdit::dateChanged, this, &MainWindow::refreshViews);
    connect(editHistoryButton, &QPushButton::clicked, this, &MainWindow::editSelectedExpense);
    connect(deleteHistoryButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedExpense);

    return page;
}

QWidget* MainWindow::buildAssetsTab()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(t(QStringLiteral("Активы и накопления"), QStringLiteral("Assets and savings")));
    title->setObjectName("pageTitle");
    root->addWidget(title);

    m_assetsSummaryLabel = new QLabel;
    m_assetsSummaryLabel->setObjectName("statusLabel");
    root->addWidget(m_assetsSummaryLabel);

    auto* assetCard = makeCard();
    auto* assetLayout = new QGridLayout(assetCard);
    assetLayout->setHorizontalSpacing(12);
    assetLayout->setVerticalSpacing(12);

    m_assetTypeBox = new QComboBox;
    for (const QString& type : defaultAssetTypes()) {
        m_assetTypeBox->addItem(displayAssetType(type), type);
    }
    m_assetTypeBox->setMinimumHeight(42);
    m_assetNameEdit = new QLineEdit;
    m_assetNameEdit->setPlaceholderText(t(QStringLiteral("SBER, BTC, вклад, облигация"), QStringLiteral("AAPL, BTC, deposit, bond")));
    m_assetNameEdit->setMinimumHeight(42);
    m_assetQuantitySpin = new QDoubleSpinBox;
    m_assetQuantitySpin->setRange(0.0001, 100000000.0);
    m_assetQuantitySpin->setDecimals(6);
    m_assetQuantitySpin->setValue(1.0);
    m_assetQuantitySpin->setMinimumHeight(42);
    m_assetPriceSpin = moneySpinBox(1000000000.0);

    m_addAssetButton = new QPushButton(t(QStringLiteral("Добавить актив"), QStringLiteral("Add asset")));
    m_addAssetButton->setObjectName("primaryButton");
    m_cancelAssetEditButton = new QPushButton(t(QStringLiteral("Отмена"), QStringLiteral("Cancel")));
    m_cancelAssetEditButton->setObjectName("toolbarButton");
    m_cancelAssetEditButton->hide();

    assetLayout->addWidget(new QLabel(t(QStringLiteral("Тип"), QStringLiteral("Type"))), 0, 0);
    assetLayout->addWidget(m_assetTypeBox, 0, 1);
    assetLayout->addWidget(new QLabel(t(QStringLiteral("Название"), QStringLiteral("Name"))), 0, 2);
    assetLayout->addWidget(m_assetNameEdit, 0, 3);
    assetLayout->addWidget(new QLabel(t(QStringLiteral("Количество"), QStringLiteral("Quantity"))), 1, 0);
    assetLayout->addWidget(m_assetQuantitySpin, 1, 1);
    assetLayout->addWidget(new QLabel(t(QStringLiteral("Цена"), QStringLiteral("Price"))), 1, 2);
    assetLayout->addWidget(m_assetPriceSpin, 1, 3);
    assetLayout->addWidget(m_addAssetButton, 1, 4);
    assetLayout->addWidget(m_cancelAssetEditButton, 0, 4);
    root->addWidget(assetCard);

    auto* splitter = new QSplitter(Qt::Horizontal);
    m_assetTable = new QTableWidget;
    m_assetTable->setColumnCount(5);
    m_assetTable->setHorizontalHeaderLabels({
        t(QStringLiteral("Тип"), QStringLiteral("Type")),
        t(QStringLiteral("Название"), QStringLiteral("Name")),
        t(QStringLiteral("Кол-во"), QStringLiteral("Qty")),
        t(QStringLiteral("Цена"), QStringLiteral("Price")),
        t(QStringLiteral("Стоимость"), QStringLiteral("Value"))
    });
    m_assetTable->horizontalHeader()->setStretchLastSection(true);
    m_assetTable->verticalHeader()->setVisible(false);
    m_assetTable->setAlternatingRowColors(true);
    m_assetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_assetTable->setSelectionMode(QAbstractItemView::SingleSelection);

    m_assetChart = new AssetChartWidget;
    m_assetChart->setTexts(
        t(QStringLiteral("Структура активов"), QStringLiteral("Asset allocation")),
        t(QStringLiteral("Добавь активы, чтобы увидеть диаграмму"), QStringLiteral("Add assets to see the chart"))
    );
    splitter->addWidget(m_assetTable);
    splitter->addWidget(m_assetChart);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter, 1);

    auto* assetActions = new QHBoxLayout;
    m_editAssetButton = new QPushButton(t(QStringLiteral("Редактировать выбранный"), QStringLiteral("Edit selected")));
    m_editAssetButton->setObjectName("toolbarButton");
    m_deleteAssetButton = new QPushButton(t(QStringLiteral("Удалить выбранный"), QStringLiteral("Delete selected")));
    m_deleteAssetButton->setObjectName("toolbarButton");
    assetActions->addStretch();
    assetActions->addWidget(m_editAssetButton);
    assetActions->addWidget(m_deleteAssetButton);
    root->addLayout(assetActions);

    connect(m_addAssetButton, &QPushButton::clicked, this, &MainWindow::addAsset);
    connect(m_cancelAssetEditButton, &QPushButton::clicked, this, &MainWindow::cancelAssetEdit);
    connect(m_editAssetButton, &QPushButton::clicked, this, &MainWindow::editSelectedAsset);
    connect(m_deleteAssetButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedAsset);

    return page;
}

QWidget* MainWindow::buildMarketsTab()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* title = new QLabel(t(QStringLiteral("Графики акций и крипты"), QStringLiteral("Stocks and crypto charts")));
    title->setObjectName("pageTitle");
    root->addWidget(title);

    auto* controls = makeCard();
    auto* layout = new QGridLayout(controls);
    layout->setHorizontalSpacing(10);
    layout->setVerticalSpacing(10);

    m_marketModeGroup = new QButtonGroup(controls);
    m_marketModeGroup->setExclusive(true);

    auto* stockModeButton = new QPushButton(t(QStringLiteral("Акции"), QStringLiteral("Stocks")));
    stockModeButton->setObjectName("chipButton");
    stockModeButton->setCheckable(true);
    stockModeButton->setChecked(m_marketMode == QStringLiteral("stock"));

    auto* cryptoModeButton = new QPushButton(t(QStringLiteral("Крипта"), QStringLiteral("Crypto")));
    cryptoModeButton->setObjectName("chipButton");
    cryptoModeButton->setCheckable(true);
    cryptoModeButton->setChecked(m_marketMode == QStringLiteral("crypto"));

    m_marketModeGroup->addButton(stockModeButton);
    m_marketModeGroup->addButton(cryptoModeButton);

    m_marketSymbolEdit = new QLineEdit;
    m_marketSymbolEdit->setMinimumHeight(42);
    m_marketSymbolEdit->setPlaceholderText(t(QStringLiteral("Например AAPL или BTC"), QStringLiteral("For example AAPL or BTC")));
    if (m_marketSymbolEdit->text().isEmpty()) {
        m_marketSymbolEdit->setText(m_marketMode == QStringLiteral("crypto") ? QStringLiteral("BTC") : QStringLiteral("AAPL"));
    }

    m_marketLoadButton = new QPushButton(t(QStringLiteral("Загрузить график"), QStringLiteral("Load chart")));
    m_marketLoadButton->setObjectName("primaryButton");

    auto setMarketMode = [this, stockModeButton, cryptoModeButton](const QString& mode) {
        m_marketMode = mode;
        stockModeButton->setChecked(mode == QStringLiteral("stock"));
        cryptoModeButton->setChecked(mode == QStringLiteral("crypto"));
        if (m_marketSymbolEdit) {
            m_marketSymbolEdit->setText(mode == QStringLiteral("crypto") ? QStringLiteral("BTC") : QStringLiteral("AAPL"));
        }
    };

    connect(stockModeButton, &QPushButton::clicked, this, [setMarketMode]() { setMarketMode(QStringLiteral("stock")); });
    connect(cryptoModeButton, &QPushButton::clicked, this, [setMarketMode]() { setMarketMode(QStringLiteral("crypto")); });

    layout->addWidget(new QLabel(t(QStringLiteral("Режим"), QStringLiteral("Mode"))), 0, 0);
    layout->addWidget(stockModeButton, 0, 1);
    layout->addWidget(cryptoModeButton, 0, 2);
    layout->addWidget(new QLabel(t(QStringLiteral("Тикер / монета"), QStringLiteral("Ticker / coin"))), 1, 0);
    layout->addWidget(m_marketSymbolEdit, 1, 1, 1, 2);
    layout->addWidget(m_marketLoadButton, 1, 3);

    auto* quickRow = new QHBoxLayout;
    const QVector<QPair<QString, QString>> quickItems = {
        {QStringLiteral("stock:AAPL"), QStringLiteral("AAPL")},
        {QStringLiteral("stock:MSFT"), QStringLiteral("MSFT")},
        {QStringLiteral("stock:NVDA"), QStringLiteral("NVDA")},
        {QStringLiteral("crypto:BTC"), QStringLiteral("BTC")},
        {QStringLiteral("crypto:ETH"), QStringLiteral("ETH")},
        {QStringLiteral("crypto:SOL"), QStringLiteral("SOL")}
    };
    for (const auto& item : quickItems) {
        auto* button = new QPushButton(item.second);
        button->setObjectName("toolbarButton");
        const QStringList parts = item.first.split(':');
        connect(button, &QPushButton::clicked, this, [this, parts, setMarketMode]() {
            setMarketMode(parts.value(0));
            m_marketSymbolEdit->setText(parts.value(1));
            loadMarketData();
        });
        quickRow->addWidget(button);
    }
    quickRow->addStretch();
    layout->addWidget(new QLabel(t(QStringLiteral("Быстрый выбор"), QStringLiteral("Quick pick"))), 2, 0);
    layout->addLayout(quickRow, 2, 1, 1, 3);
    root->addWidget(controls);

    m_marketStatusLabel = new QLabel(t(
        QStringLiteral("Акции: Yahoo Finance API. Крипта: CoinGecko API."),
        QStringLiteral("Stocks: Yahoo Finance API. Crypto: CoinGecko API.")
    ));
    m_marketStatusLabel->setObjectName("statusLabel");
    root->addWidget(m_marketStatusLabel);

    m_marketChart = new MarketChartWidget;
    m_marketChart->setEmptyText(t(
        QStringLiteral("Выбери инструмент и нажми загрузить"),
        QStringLiteral("Choose an instrument and load the chart")
    ));
    root->addWidget(m_marketChart, 1);

    connect(m_marketLoadButton, &QPushButton::clicked, this, &MainWindow::loadMarketData);

    return page;
}

QWidget* MainWindow::buildInsiderTab()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto* topRow = new QHBoxLayout;
    auto* title = new QLabel(t(QStringLiteral("Инсайдерские сделки"), QStringLiteral("Insider trades")));
    title->setObjectName("pageTitle");
    m_loadInsidersButton = new QPushButton(t(QStringLiteral("Загрузить сделки"), QStringLiteral("Load trades")));
    m_loadInsidersButton->setObjectName("primaryButton");
    topRow->addWidget(title, 1);
    topRow->addWidget(m_loadInsidersButton);
    root->addLayout(topRow);

    m_insiderStatusLabel = new QLabel(t(
        QStringLiteral("Источник: SEC EDGAR Form 4. Если сеть недоступна, появятся демо-данные."),
        QStringLiteral("Source: SEC EDGAR Form 4. Demo data appears when network is unavailable.")
    ));
    m_insiderStatusLabel->setObjectName("statusLabel");
    root->addWidget(m_insiderStatusLabel);

    m_insiderTable = new QTableWidget;
    m_insiderTable->setColumnCount(9);
    m_insiderTable->setHorizontalHeaderLabels({
        QStringLiteral("Filing"),
        QStringLiteral("Trade"),
        QStringLiteral("Ticker"),
        QStringLiteral("Company"),
        QStringLiteral("Insider"),
        QStringLiteral("Role"),
        QStringLiteral("Type"),
        QStringLiteral("Price"),
        QStringLiteral("Value")
    });
    m_insiderTable->horizontalHeader()->setStretchLastSection(true);
    m_insiderTable->verticalHeader()->setVisible(false);
    m_insiderTable->setAlternatingRowColors(true);
    root->addWidget(m_insiderTable, 1);

    connect(m_loadInsidersButton, &QPushButton::clicked, this, &MainWindow::refreshInsiderTrades);

    return page;
}

QWidget* MainWindow::buildEducationTab()
{
    auto* page = new QWidget;
    auto* root = new QHBoxLayout(page);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(18);

    auto* leftCard = makeCard();
    leftCard->setMinimumWidth(290);
    auto* leftLayout = new QVBoxLayout(leftCard);
    auto* title = new QLabel(t(QStringLiteral("Финансовая грамотность"), QStringLiteral("Financial literacy")));
    title->setObjectName("sectionTitle");
    leftLayout->addWidget(title);

    m_courseList = new QListWidget;
    m_courseList->setObjectName("courseList");
    for (const Course& course : m_courses) {
        m_courseList->addItem(course.title + "\n" + course.subtitle);
    }
    m_courseList->setCurrentRow(qBound(0, m_currentCourseIndex, m_courses.size() - 1));
    leftLayout->addWidget(m_courseList, 1);
    root->addWidget(leftCard);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setObjectName("courseScroll");
    auto* content = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(14);

    auto* headerCard = makeCard();
    auto* headerLayout = new QVBoxLayout(headerCard);
    m_courseTitleLabel = new QLabel;
    m_courseTitleLabel->setObjectName("pageTitle");
    m_courseSubtitleLabel = new QLabel;
    m_courseSubtitleLabel->setObjectName("statusLabel");
    headerLayout->addWidget(m_courseTitleLabel);
    headerLayout->addWidget(m_courseSubtitleLabel);
    contentLayout->addWidget(headerCard);

    m_courseTheoryText = new QTextEdit;
    m_courseTheoryText->setReadOnly(true);
    m_courseTheoryText->setMinimumHeight(260);
    m_courseTheoryText->setObjectName("theoryText");
    contentLayout->addWidget(m_courseTheoryText);

    for (int i = 0; i < 10; ++i) {
        auto* questionCard = makeCard();
        auto* questionLayout = new QVBoxLayout(questionCard);
        auto* questionLabel = new QLabel;
        questionLabel->setWordWrap(true);
        questionLabel->setObjectName("questionLabel");
        questionLayout->addWidget(questionLabel);

        auto* group = new QButtonGroup(questionCard);
        QVector<QRadioButton*> buttons;
        for (int j = 0; j < 3; ++j) {
            auto* radio = new QRadioButton;
            radio->setMinimumHeight(28);
            group->addButton(radio, j);
            buttons.push_back(radio);
            questionLayout->addWidget(radio);
        }

        m_quizQuestionLabels.push_back(questionLabel);
        m_quizGroups.push_back(group);
        m_quizButtons.push_back(buttons);
        contentLayout->addWidget(questionCard);
    }

    auto* bottom = new QHBoxLayout;
    m_submitQuizButton = new QPushButton(t(QStringLiteral("Проверить тест"), QStringLiteral("Check quiz")));
    m_submitQuizButton->setObjectName("primaryButton");
    m_quizResultLabel = new QLabel;
    m_quizResultLabel->setObjectName("certificateLabel");
    m_quizResultLabel->setWordWrap(true);
    bottom->addWidget(m_submitQuizButton);
    bottom->addWidget(m_quizResultLabel, 1);
    contentLayout->addLayout(bottom);
    contentLayout->addStretch();

    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    connect(m_courseList, &QListWidget::currentRowChanged, this, &MainWindow::selectCourse);
    connect(m_submitQuizButton, &QPushButton::clicked, this, &MainWindow::submitQuiz);

    return page;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_reportFromDateEdit || watched == m_reportToDateEdit)
        && event->type() == QEvent::MouseButtonPress) {
        showCalendarPopup(qobject_cast<QDateEdit*>(watched));
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::showCalendarPopup(QDateEdit* target)
{
    if (!target) {
        return;
    }

    auto* popup = new QFrame(nullptr, Qt::Popup);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setObjectName("calendarPopup");

    auto* layout = new QVBoxLayout(popup);
    layout->setContentsMargins(8, 8, 8, 8);

    auto* calendar = new QCalendarWidget(popup);
    calendar->setGridVisible(true);
    calendar->setSelectedDate(target->date());
    calendar->setMinimumDate(target->minimumDate());
    calendar->setMaximumDate(target->maximumDate());
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    layout->addWidget(calendar);

    connect(calendar, &QCalendarWidget::clicked, this, [target, popup](const QDate& date) {
        target->setDate(date);
        popup->close();
    });

    popup->adjustSize();
    popup->move(target->mapToGlobal(QPoint(0, target->height() + 4)));
    popup->show();
}

void MainWindow::applyTheme()
{
    if (!m_darkMode) {
        setStyleSheet(QStringLiteral(R"(
            QMainWindow, QWidget { background: #eef2f7; color: #111827; font-family: "Segoe UI", Arial; font-size: 10pt; }
            QTabWidget::pane { border: 0; }
            QTabBar::tab { background: #dbe4ef; color: #334155; padding: 12px 20px; margin-right: 6px; border-radius: 8px; min-width: 92px; }
            QTabBar::tab:selected { background: #2563eb; color: white; }
            QFrame#card { background: #ffffff; border: 1px solid #d9e2ef; border-radius: 8px; }
            QLabel#appTitle { font-size: 15pt; font-weight: 800; color: #0f172a; }
            QLabel#pageTitle { font-size: 22pt; font-weight: 800; color: #0f172a; }
            QLabel#sectionTitle { font-size: 14pt; font-weight: 700; color: #0f172a; }
            QLabel#metricMain { font-size: 22pt; font-weight: 800; color: #0f172a; }
            QLabel#statusLabel { color: #475569; font-size: 10pt; }
            QLabel#questionLabel { font-size: 11pt; font-weight: 700; color: #0f172a; }
            QLabel#certificateLabel { color: #166534; font-size: 11pt; font-weight: 700; }
            QPushButton { background: #e2e8f0; border: 1px solid #cbd5e1; border-radius: 10px; padding: 10px 16px; min-height: 28px; }
            QPushButton:hover { background: #cbd5e1; }
            QPushButton#primaryButton { background: #2563eb; color: white; border: 1px solid #1d4ed8; font-weight: 700; min-height: 38px; padding: 11px 18px; }
            QPushButton#primaryButton:hover { background: #1d4ed8; }
            QPushButton#toolbarButton { background: #ffffff; border: 1px solid #cbd5e1; padding: 8px 12px; }
            QPushButton#topToggleButton { background: #ffffff; color: #334155; border: 1px solid #cbd5e1; border-radius: 9px; min-width: 92px; max-width: 92px; min-height: 34px; max-height: 34px; padding: 0; font-weight: 700; }
            QPushButton#topToggleButton:hover { background: #e0f2fe; border-color: #7dd3fc; }
            QPushButton#chipButton { background: #f8fafc; color: #334155; border: 1px solid #cbd5e1; border-radius: 17px; padding: 8px 14px; min-height: 28px; font-weight: 600; }
            QPushButton#chipButton:hover { background: #e0f2fe; border-color: #7dd3fc; }
            QPushButton#chipButton:checked { background: #dbeafe; color: #1d4ed8; border: 1px solid #2563eb; font-weight: 800; }
            QLineEdit, QComboBox, QDoubleSpinBox, QDateEdit { background: white; border: 1px solid #cbd5e1; border-radius: 10px; padding: 8px 12px; selection-background-color: #bfdbfe; }
            QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QDateEdit:focus { border: 1px solid #2563eb; }
            QComboBox::drop-down, QDateEdit::drop-down { border: 0; width: 26px; }
            QTableWidget { background: white; alternate-background-color: #f8fafc; border: 1px solid #d9e2ef; border-radius: 8px; gridline-color: #e2e8f0; }
            QHeaderView::section { background: #e2e8f0; color: #334155; font-weight: 700; padding: 8px; border: 0; }
            QFrame#calendarPopup { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 10px; }
            QCalendarWidget QWidget { background: #ffffff; color: #111827; }
            QCalendarWidget QToolButton { background: #f8fafc; border: 1px solid #cbd5e1; border-radius: 7px; padding: 5px 8px; }
            QCalendarWidget QAbstractItemView { background: #ffffff; selection-background-color: #2563eb; selection-color: #ffffff; outline: 0; }
            QListWidget#courseList { background: white; border: 1px solid #d9e2ef; border-radius: 8px; outline: 0; }
            QListWidget#courseList::item { padding: 12px; border-bottom: 1px solid #e2e8f0; }
            QListWidget#courseList::item:selected { background: #dbeafe; color: #1e3a8a; }
            QTextEdit#theoryText { background: #ffffff; color: #111827; border: 1px solid #d9e2ef; border-radius: 8px; padding: 14px; }
            QScrollArea#courseScroll { border: 0; background: transparent; }
        )"));
    } else {
        setStyleSheet(QStringLiteral(R"(
            QMainWindow, QWidget { background: #0f172a; color: #e5e7eb; font-family: "Segoe UI", Arial; font-size: 10pt; }
            QTabWidget::pane { border: 0; }
            QTabBar::tab { background: #1e293b; color: #cbd5e1; padding: 12px 20px; margin-right: 6px; border-radius: 8px; min-width: 92px; }
            QTabBar::tab:selected { background: #38bdf8; color: #082f49; }
            QFrame#card { background: #172033; border: 1px solid #334155; border-radius: 8px; }
            QLabel#appTitle { font-size: 15pt; font-weight: 800; color: #f8fafc; }
            QLabel#pageTitle { font-size: 22pt; font-weight: 800; color: #f8fafc; }
            QLabel#sectionTitle { font-size: 14pt; font-weight: 700; color: #f8fafc; }
            QLabel#metricMain { font-size: 22pt; font-weight: 800; color: #f8fafc; }
            QLabel#statusLabel { color: #cbd5e1; font-size: 10pt; }
            QLabel#questionLabel { font-size: 11pt; font-weight: 700; color: #f8fafc; }
            QLabel#certificateLabel { color: #86efac; font-size: 11pt; font-weight: 700; }
            QPushButton { background: #253348; color: #e5e7eb; border: 1px solid #475569; border-radius: 10px; padding: 10px 16px; min-height: 28px; }
            QPushButton:hover { background: #334155; }
            QPushButton#primaryButton { background: #38bdf8; color: #082f49; border: 1px solid #0ea5e9; font-weight: 800; min-height: 38px; padding: 11px 18px; }
            QPushButton#primaryButton:hover { background: #7dd3fc; }
            QPushButton#toolbarButton { background: #172033; border: 1px solid #475569; padding: 8px 12px; }
            QPushButton#topToggleButton { background: #172033; color: #e5e7eb; border: 1px solid #475569; border-radius: 9px; min-width: 92px; max-width: 92px; min-height: 34px; max-height: 34px; padding: 0; font-weight: 700; }
            QPushButton#topToggleButton:hover { background: #253348; border-color: #38bdf8; }
            QPushButton#chipButton { background: #111827; color: #cbd5e1; border: 1px solid #475569; border-radius: 17px; padding: 8px 14px; min-height: 28px; font-weight: 600; }
            QPushButton#chipButton:hover { background: #172033; border-color: #38bdf8; }
            QPushButton#chipButton:checked { background: #0f3b56; color: #7dd3fc; border: 1px solid #38bdf8; font-weight: 800; }
            QLineEdit, QComboBox, QDoubleSpinBox, QDateEdit { background: #111827; color: #f8fafc; border: 1px solid #475569; border-radius: 10px; padding: 8px 12px; selection-background-color: #0ea5e9; }
            QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus, QDateEdit:focus { border: 1px solid #38bdf8; }
            QComboBox::drop-down, QDateEdit::drop-down { border: 0; width: 26px; }
            QTableWidget { background: #111827; color: #e5e7eb; alternate-background-color: #172033; border: 1px solid #334155; border-radius: 8px; gridline-color: #334155; }
            QHeaderView::section { background: #1e293b; color: #e5e7eb; font-weight: 700; padding: 8px; border: 0; }
            QFrame#calendarPopup { background: #111827; border: 1px solid #475569; border-radius: 10px; }
            QCalendarWidget QWidget { background: #111827; color: #e5e7eb; }
            QCalendarWidget QToolButton { background: #172033; color: #e5e7eb; border: 1px solid #475569; border-radius: 7px; padding: 5px 8px; }
            QCalendarWidget QAbstractItemView { background: #111827; color: #e5e7eb; selection-background-color: #38bdf8; selection-color: #082f49; outline: 0; }
            QListWidget#courseList { background: #111827; color: #e5e7eb; border: 1px solid #334155; border-radius: 8px; outline: 0; }
            QListWidget#courseList::item { padding: 12px; border-bottom: 1px solid #334155; }
            QListWidget#courseList::item:selected { background: #0f3b56; color: #e0f2fe; }
            QTextEdit#theoryText { background: #111827; color: #e5e7eb; border: 1px solid #334155; border-radius: 8px; padding: 14px; }
            QScrollArea#courseScroll { border: 0; background: transparent; }
        )"));
    }
}

void MainWindow::toggleTheme()
{
    m_darkMode = !m_darkMode;
    applyTheme();
    if (m_themeButton) {
        m_themeButton->setText(m_darkMode ? t(QStringLiteral("Светлая"), QStringLiteral("Light"))
                                          : t(QStringLiteral("Темная"), QStringLiteral("Dark")));
    }
}

void MainWindow::toggleLanguage()
{
    m_language = m_language == UiLanguage::English ? UiLanguage::Russian : UiLanguage::English;
    rebuildUi();
}

void MainWindow::saveDailyLimit()
{
    m_manager.setDailyLimit(m_dailyLimitSpin->value());
}

void MainWindow::addExpense()
{
    Expense expense;
    expense.date = m_expenseDateEdit->date();
    expense.category = m_selectedExpenseCategory.isEmpty()
        ? m_manager.budgets().first().category
        : m_selectedExpenseCategory;
    expense.title = m_expenseTitleEdit->text().trimmed();
    if (expense.title.isEmpty()) {
        expense.title = t(QStringLiteral("Расход"), QStringLiteral("Expense"));
    }
    expense.amount = m_expenseAmountSpin->value();

    if (m_editingExpenseIndex >= 0) {
        m_manager.updateExpense(m_editingExpenseIndex, expense);
        resetExpenseEditor();
    } else {
        m_manager.addExpense(expense);
        m_expenseTitleEdit->clear();
    }
}

void MainWindow::editSelectedExpense()
{
    int index = -1;
    if (m_tabs && m_tabs->currentIndex() == 1 && m_reportExpenseTable) {
        const int row = m_reportExpenseTable->currentRow();
        if (row >= 0 && row < m_reportExpenseIndexes.size()) {
            index = m_reportExpenseIndexes.at(row);
        }
    } else if (m_todayExpenseTable) {
        const int row = m_todayExpenseTable->currentRow();
        if (row >= 0 && row < m_todayExpenseIndexes.size()) {
            index = m_todayExpenseIndexes.at(row);
        }
    }

    if (index < 0) {
        m_todayStatusLabel->setText(t(QStringLiteral("Выбери трату в таблице, чтобы ее редактировать."), QStringLiteral("Select an expense row to edit.")));
        return;
    }

    startExpenseEdit(index);
}

void MainWindow::deleteSelectedExpense()
{
    int index = -1;
    if (m_tabs && m_tabs->currentIndex() == 1 && m_reportExpenseTable) {
        const int row = m_reportExpenseTable->currentRow();
        if (row >= 0 && row < m_reportExpenseIndexes.size()) {
            index = m_reportExpenseIndexes.at(row);
        }
    } else if (m_todayExpenseTable) {
        const int row = m_todayExpenseTable->currentRow();
        if (row >= 0 && row < m_todayExpenseIndexes.size()) {
            index = m_todayExpenseIndexes.at(row);
        }
    }

    if (index < 0) {
        m_todayStatusLabel->setText(t(QStringLiteral("Выбери трату в таблице, чтобы ее удалить."), QStringLiteral("Select an expense row to delete.")));
        return;
    }

    if (m_manager.removeExpense(index)) {
        resetExpenseEditor();
        m_todayStatusLabel->setText(t(QStringLiteral("Трата удалена."), QStringLiteral("Expense deleted.")));
    }
}

void MainWindow::cancelExpenseEdit()
{
    resetExpenseEditor();
}

void MainWindow::addAsset()
{
    Asset asset;
    asset.type = m_assetTypeBox->currentData().toString();
    asset.name = m_assetNameEdit->text().trimmed();
    if (asset.name.isEmpty()) {
        asset.name = t(QStringLiteral("Актив"), QStringLiteral("Asset"));
    }
    asset.quantity = m_assetQuantitySpin->value();
    asset.price = m_assetPriceSpin->value();

    if (m_editingAssetIndex >= 0) {
        m_manager.updateAsset(m_editingAssetIndex, asset);
        resetAssetEditor();
    } else {
        m_manager.addAsset(asset);
        m_assetNameEdit->clear();
    }
}

void MainWindow::editSelectedAsset()
{
    const int row = m_assetTable ? m_assetTable->currentRow() : -1;
    if (row < 0 || row >= m_assetIndexes.size()) {
        m_assetsSummaryLabel->setText(t(QStringLiteral("Выбери актив в таблице, чтобы его редактировать."), QStringLiteral("Select an asset row to edit.")));
        return;
    }

    startAssetEdit(m_assetIndexes.at(row));
}

void MainWindow::deleteSelectedAsset()
{
    const int row = m_assetTable ? m_assetTable->currentRow() : -1;
    if (row < 0 || row >= m_assetIndexes.size()) {
        m_assetsSummaryLabel->setText(t(QStringLiteral("Выбери актив в таблице, чтобы его удалить."), QStringLiteral("Select an asset row to delete.")));
        return;
    }

    const int index = m_assetIndexes.at(row);
    if (m_manager.removeAsset(index)) {
        resetAssetEditor();
        m_assetsSummaryLabel->setText(t(QStringLiteral("Актив удален."), QStringLiteral("Asset deleted.")));
    }
}

void MainWindow::cancelAssetEdit()
{
    resetAssetEditor();
}

void MainWindow::loadMarketData()
{
    const QString symbol = m_marketSymbolEdit ? m_marketSymbolEdit->text().trimmed() : QString();
    if (m_marketLoadButton) {
        m_marketLoadButton->setEnabled(false);
    }
    if (m_marketStatusLabel) {
        m_marketStatusLabel->setText(t(QStringLiteral("Загрузка рыночных данных..."), QStringLiteral("Loading market data...")));
        m_marketStatusLabel->setStyleSheet(QString());
    }

    if (m_marketMode == QStringLiteral("crypto")) {
        m_marketService.fetchCrypto(symbol);
    } else {
        m_marketService.fetchStock(symbol);
    }
}

void MainWindow::showMarketData(const QString& title, const QVector<MarketPoint>& points, const QString& message, bool usedFallback)
{
    if (m_marketChart) {
        m_marketChart->setSeries(title, points, QStringLiteral("USD"));
    }
    if (m_marketStatusLabel) {
        m_marketStatusLabel->setText(m_language == UiLanguage::English
            ? (usedFallback ? QStringLiteral("API unavailable, demo chart is shown.") : QStringLiteral("Market data loaded from API."))
            : message);
        m_marketStatusLabel->setStyleSheet(usedFallback ? "color: #b45309;" : "color: #166534;");
    }
    if (m_marketLoadButton) {
        m_marketLoadButton->setEnabled(true);
    }
}

void MainWindow::refreshInsiderTrades()
{
    m_loadInsidersButton->setEnabled(false);
    m_insiderStatusLabel->setText(t(QStringLiteral("Загрузка данных..."), QStringLiteral("Loading data...")));
    m_insiderService.fetchLargeTrades();
}

void MainWindow::showInsiderTrades(const QVector<InsiderTrade>& trades, const QString& message, bool usedFallback)
{
    m_insiderTable->setRowCount(trades.size());
    for (int row = 0; row < trades.size(); ++row) {
        const InsiderTrade& trade = trades.at(row);
        m_insiderTable->setItem(row, 0, tableItem(trade.filingDate));
        m_insiderTable->setItem(row, 1, tableItem(trade.tradeDate));
        m_insiderTable->setItem(row, 2, tableItem(trade.ticker));
        m_insiderTable->setItem(row, 3, tableItem(trade.company));
        m_insiderTable->setItem(row, 4, tableItem(trade.insider));
        m_insiderTable->setItem(row, 5, tableItem(trade.role));
        m_insiderTable->setItem(row, 6, tableItem(trade.tradeType));
        m_insiderTable->setItem(row, 7, tableItem(trade.price));
        m_insiderTable->setItem(row, 8, tableItem(trade.value));
    }
    m_insiderTable->resizeColumnsToContents();
    m_insiderStatusLabel->setText(m_language == UiLanguage::English
        ? (usedFallback ? QStringLiteral("Network or parsing unavailable, demo data is shown.")
                        : QStringLiteral("Loaded recent Form 4 filings from SEC EDGAR."))
        : message);
    m_insiderStatusLabel->setStyleSheet(usedFallback ? "color: #b45309;" : "color: #166534;");
    m_loadInsidersButton->setEnabled(true);
}

void MainWindow::selectCourse(int row)
{
    if (row >= 0 && row < m_courses.size()) {
        renderCourse(row);
    }
}

void MainWindow::submitQuiz()
{
    if (m_currentCourseIndex < 0 || m_currentCourseIndex >= m_courses.size()) {
        return;
    }

    const Course& course = m_courses.at(m_currentCourseIndex);
    int correct = 0;
    for (int i = 0; i < course.questions.size(); ++i) {
        if (m_quizGroups.at(i)->checkedId() == course.questions.at(i).correctIndex) {
            ++correct;
        }
    }

    const int percent = qRound(correct * 100.0 / course.questions.size());
    if (percent >= 80) {
        m_quizResultLabel->setStyleSheet(m_darkMode ? "color: #86efac;" : "color: #166534;");
        m_quizResultLabel->setText(t(
            QStringLiteral("Сертификат выдан: %1. Результат %2/10 (%3%)."),
            QStringLiteral("Certificate issued: %1. Score %2/10 (%3%).")
        ).arg(course.title).arg(correct).arg(percent));
    } else {
        m_quizResultLabel->setStyleSheet("color: #b45309;");
        m_quizResultLabel->setText(t(
            QStringLiteral("Результат %1/10 (%2%). Для сертификата нужно 80% и выше."),
            QStringLiteral("Score %1/10 (%2%). Certificate requires 80% or higher.")
        ).arg(correct).arg(percent));
    }
}

void MainWindow::refreshViews()
{
    updateCategoryCombos();
    refreshTodayPanel();
    refreshExpenseTable();
    refreshReportTable();
    refreshAssetTable();
}

void MainWindow::updateCategoryCombos()
{
    if (m_selectedExpenseCategory.isEmpty() && !m_manager.budgets().isEmpty()) {
        m_selectedExpenseCategory = m_manager.budgets().first().category;
    }

    if (!m_expenseCategoryGroup) {
        return;
    }

    for (QAbstractButton* abstractButton : m_expenseCategoryGroup->buttons()) {
        auto* button = qobject_cast<QPushButton*>(abstractButton);
        if (!button) {
            continue;
        }
        const QString category = button->property("category").toString();
        button->setText(displayCategory(category));
        button->setChecked(category == m_selectedExpenseCategory);
    }
}

void MainWindow::setExpenseCategory(const QString& category)
{
    m_selectedExpenseCategory = category;
    if (!m_expenseCategoryGroup) {
        return;
    }

    for (QAbstractButton* abstractButton : m_expenseCategoryGroup->buttons()) {
        auto* button = qobject_cast<QPushButton*>(abstractButton);
        if (!button) {
            continue;
        }
        button->setChecked(button->property("category").toString() == category);
    }
}

void MainWindow::startExpenseEdit(int index)
{
    if (index < 0 || index >= m_manager.expenses().size()) {
        return;
    }

    const Expense& expense = m_manager.expenses().at(index);
    m_editingExpenseIndex = index;
    m_expenseDateEdit->setDate(expense.date);
    m_expenseTitleEdit->setText(expense.title);
    m_expenseAmountSpin->setValue(expense.amount);
    setExpenseCategory(expense.category);

    if (m_addExpenseButton) {
        m_addExpenseButton->setText(t(QStringLiteral("Сохранить трату"), QStringLiteral("Save expense")));
    }
    if (m_cancelExpenseEditButton) {
        m_cancelExpenseEditButton->show();
    }
    if (m_tabs) {
        m_tabs->setCurrentIndex(0);
    }
    m_todayStatusLabel->setText(t(QStringLiteral("Режим редактирования траты."), QStringLiteral("Expense edit mode.")));
}

void MainWindow::resetExpenseEditor()
{
    m_editingExpenseIndex = -1;
    if (m_expenseDateEdit) {
        m_expenseDateEdit->setDate(QDate::currentDate());
    }
    if (m_expenseTitleEdit) {
        m_expenseTitleEdit->clear();
    }
    if (m_expenseAmountSpin) {
        m_expenseAmountSpin->setValue(0.01);
    }
    if (m_addExpenseButton) {
        m_addExpenseButton->setText(t(QStringLiteral("Добавить трату"), QStringLiteral("Add expense")));
    }
    if (m_cancelExpenseEditButton) {
        m_cancelExpenseEditButton->hide();
    }
}

void MainWindow::startAssetEdit(int index)
{
    if (index < 0 || index >= m_manager.assets().size()) {
        return;
    }

    const Asset& asset = m_manager.assets().at(index);
    m_editingAssetIndex = index;
    const int typeIndex = m_assetTypeBox->findData(asset.type);
    if (typeIndex >= 0) {
        m_assetTypeBox->setCurrentIndex(typeIndex);
    }
    m_assetNameEdit->setText(asset.name);
    m_assetQuantitySpin->setValue(asset.quantity);
    m_assetPriceSpin->setValue(asset.price);

    if (m_addAssetButton) {
        m_addAssetButton->setText(t(QStringLiteral("Сохранить актив"), QStringLiteral("Save asset")));
    }
    if (m_cancelAssetEditButton) {
        m_cancelAssetEditButton->show();
    }
    m_assetsSummaryLabel->setText(t(QStringLiteral("Режим редактирования актива."), QStringLiteral("Asset edit mode.")));
}

void MainWindow::resetAssetEditor()
{
    m_editingAssetIndex = -1;
    if (m_assetNameEdit) {
        m_assetNameEdit->clear();
    }
    if (m_assetQuantitySpin) {
        m_assetQuantitySpin->setValue(1.0);
    }
    if (m_assetPriceSpin) {
        m_assetPriceSpin->setValue(0.0);
    }
    if (m_addAssetButton) {
        m_addAssetButton->setText(t(QStringLiteral("Добавить актив"), QStringLiteral("Add asset")));
    }
    if (m_cancelAssetEditButton) {
        m_cancelAssetEditButton->hide();
    }
}

void MainWindow::refreshTodayPanel()
{
    m_dailyLimitSpin->setValue(m_manager.dailyLimit());

    const double spent = m_manager.totalSpent();
    const double remaining = m_manager.totalRemaining();
    m_todaySpentLabel->setText(formatMoney(spent));
    m_todayLimitLabel->setText(formatMoney(m_manager.dailyLimit()));
    m_todayRemainingLabel->setText(formatMoney(remaining));

    if (remaining < 0.0) {
        m_todayRemainingLabel->setStyleSheet("color: #dc2626;");
        m_todayStatusLabel->setText(t(
            QStringLiteral("Лимит превышен. Счет ушел в минус на %1."),
            QStringLiteral("Limit exceeded. Balance is negative by %1.")
        ).arg(formatMoney(qAbs(remaining))));
    } else {
        m_todayRemainingLabel->setStyleSheet(m_darkMode ? "color: #86efac;" : "color: #16a34a;");
        m_todayStatusLabel->setText(t(
            QStringLiteral("Сегодня можно потратить еще %1."),
            QStringLiteral("You can still spend %1 today.")
        ).arg(formatMoney(remaining)));
    }
}

void MainWindow::refreshExpenseTable()
{
    const QVector<Expense>& expenses = m_manager.expenses();
    m_todayExpenseIndexes.clear();
    int rowCount = 0;
    for (const Expense& expense : expenses) {
        if (expense.date == QDate::currentDate()) {
            ++rowCount;
        }
    }

    m_todayExpenseTable->setRowCount(rowCount);
    int row = 0;
    for (int i = 0; i < expenses.size(); ++i) {
        const Expense& expense = expenses.at(i);
        if (expense.date != QDate::currentDate()) {
            continue;
        }
        m_todayExpenseIndexes.push_back(i);
        m_todayExpenseTable->setItem(row, 0, tableItem(expense.date.toString("dd.MM.yyyy")));
        m_todayExpenseTable->setItem(row, 1, tableItem(displayCategory(expense.category)));
        m_todayExpenseTable->setItem(row, 2, tableItem(expense.title));
        m_todayExpenseTable->setItem(row, 3, tableItem(formatMoney(expense.amount)));
        ++row;
    }
    m_todayExpenseTable->resizeColumnsToContents();
}

void MainWindow::refreshReportTable()
{
    QDate fromDate = m_reportFromDateEdit->date();
    QDate toDate = m_reportToDateEdit->date();
    if (fromDate > toDate) {
        qSwap(fromDate, toDate);
    }

    m_reportExpenseIndexes.clear();
    QVector<CategoryBudget> rows;
    for (const CategoryBudget& budget : m_manager.budgets()) {
        rows.push_back({budget.category, 0.0, 0.0});
    }

    double total = 0.0;
    for (const Expense& expense : m_manager.expenses()) {
        if (expense.date < fromDate || expense.date > toDate) {
            continue;
        }
        total += expense.amount;
        bool found = false;
        for (CategoryBudget& row : rows) {
            if (row.category == expense.category) {
                row.spent += expense.amount;
                found = true;
                break;
            }
        }
        if (!found) {
            rows.push_back({expense.category, 0.0, expense.amount});
        }
    }

    m_reportSummaryLabel->setText(t(
        QStringLiteral("Всего за период: %1"),
        QStringLiteral("Total for period: %1")
    ).arg(formatMoney(total)));
    m_reportTable->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        const CategoryBudget& item = rows.at(row);
        const double percent = total > 0.0 ? item.spent / total * 100.0 : 0.0;
        m_reportTable->setItem(row, 0, tableItem(displayCategory(item.category)));
        m_reportTable->setItem(row, 1, tableItem(formatMoney(item.spent)));
        m_reportTable->setItem(row, 2, tableItem(QString::number(percent, 'f', 1) + "%"));
    }
    m_reportTable->resizeColumnsToContents();
    m_budgetChart->setBudgets(rows);

    int historyRows = 0;
    for (const Expense& expense : m_manager.expenses()) {
        if (expense.date >= fromDate && expense.date <= toDate) {
            ++historyRows;
        }
    }

    m_reportExpenseTable->setRowCount(historyRows);
    int historyRow = 0;
    const QVector<Expense>& expenses = m_manager.expenses();
    for (int i = 0; i < expenses.size(); ++i) {
        const Expense& expense = expenses.at(i);
        if (expense.date < fromDate || expense.date > toDate) {
            continue;
        }
        m_reportExpenseIndexes.push_back(i);
        m_reportExpenseTable->setItem(historyRow, 0, tableItem(expense.date.toString("dd.MM.yyyy")));
        m_reportExpenseTable->setItem(historyRow, 1, tableItem(displayCategory(expense.category)));
        m_reportExpenseTable->setItem(historyRow, 2, tableItem(expense.title));
        m_reportExpenseTable->setItem(historyRow, 3, tableItem(formatMoney(expense.amount)));
        ++historyRow;
    }
    m_reportExpenseTable->resizeColumnsToContents();
}

void MainWindow::refreshAssetTable()
{
    const QVector<Asset>& assets = m_manager.assets();
    m_assetIndexes.clear();
    m_assetTable->setRowCount(assets.size());
    for (int row = 0; row < assets.size(); ++row) {
        const Asset& asset = assets.at(row);
        m_assetIndexes.push_back(row);
        m_assetTable->setItem(row, 0, tableItem(displayAssetType(asset.type)));
        m_assetTable->setItem(row, 1, tableItem(asset.name));
        m_assetTable->setItem(row, 2, tableItem(QString::number(asset.quantity, 'f', 6)));
        m_assetTable->setItem(row, 3, tableItem(formatMoney(asset.price)));
        m_assetTable->setItem(row, 4, tableItem(formatMoney(asset.marketValue())));
    }
    m_assetTable->resizeColumnsToContents();
    m_assetChart->setAssets(assets);
    m_assetsSummaryLabel->setText(t(
        QStringLiteral("Общая стоимость активов: %1"),
        QStringLiteral("Total asset value: %1")
    ).arg(formatMoney(m_manager.totalAssetsValue())));
}

void MainWindow::renderCourse(int index)
{
    m_currentCourseIndex = index;
    const Course& course = m_courses.at(index);
    m_courseTitleLabel->setText(course.title);
    m_courseSubtitleLabel->setText(course.subtitle);
    m_courseTheoryText->setHtml(course.theory);
    m_quizResultLabel->clear();

    for (int i = 0; i < course.questions.size(); ++i) {
        const QuizQuestion& question = course.questions.at(i);
        m_quizQuestionLabels.at(i)->setText(QString("%1. %2").arg(i + 1).arg(question.text));
        m_quizGroups.at(i)->setExclusive(false);
        for (int j = 0; j < question.answers.size(); ++j) {
            m_quizButtons.at(i).at(j)->setText(question.answers.at(j));
            m_quizButtons.at(i).at(j)->setChecked(false);
        }
        m_quizGroups.at(i)->setExclusive(true);
    }
}

QVector<Course> MainWindow::createCourses() const
{
    if (m_language == UiLanguage::English) {
        return {
            {
                QStringLiteral("How to Save Money"),
                QStringLiteral("Emergency fund, consistency, and a simple daily budget"),
                QStringLiteral("<h2>How to save money</h2><p>Saving starts with a rule, not with a huge salary. Move a fixed amount or percentage into savings right after income arrives. This is called paying yourself first.</p><p>An emergency fund is the base of personal finance. It is usually measured in months of required spending: food, rent, transport, internet, medicine. A good starter target is three months.</p><p>A daily limit turns budgeting into a simple signal. If the limit is 500 RUB and you spend 540 RUB, the remaining balance is -40 RUB. That means tomorrow you should spend less or adjust the plan.</p><p>Separate savings by goals: emergency fund, big purchase, investments. Money needed soon should not be placed in risky assets.</p>"),
                {
                    {QStringLiteral("What does 'pay yourself first' mean?"), {QStringLiteral("Save first, spend after"), QStringLiteral("Pay friends first"), QStringLiteral("Spend everything and save leftovers")}, 0},
                    {QStringLiteral("Why do you need an emergency fund?"), {QStringLiteral("For unexpected required expenses and income loss"), QStringLiteral("For leveraged trading"), QStringLiteral("For the riskiest assets")}, 0},
                    {QStringLiteral("How is an emergency fund usually measured?"), {QStringLiteral("In months of required expenses"), QStringLiteral("In number of bank cards"), QStringLiteral("In stock percentages")}, 0},
                    {QStringLiteral("What does a negative daily balance mean?"), {QStringLiteral("You spent more than planned"), QStringLiteral("Income increased"), QStringLiteral("The limit is infinite")}, 0},
                    {QStringLiteral("Where should short-term required money be kept?"), {QStringLiteral("In a reliable and liquid place"), QStringLiteral("Only in futures"), QStringLiteral("In random crypto")}, 0},
                    {QStringLiteral("Why is a small regular amount useful?"), {QStringLiteral("It builds a habit and reduces pressure"), QStringLiteral("It guarantees huge returns"), QStringLiteral("It removes expense tracking")}, 0},
                    {QStringLiteral("What is better for savings?"), {QStringLiteral("Separate them by goals"), QStringLiteral("Mix everything together"), QStringLiteral("Always keep all money in cash")}, 0},
                    {QStringLiteral("Which expenses are required?"), {QStringLiteral("Food, rent, transport, communication"), QStringLiteral("Any entertainment"), QStringLiteral("Only investments")}, 0},
                    {QStringLiteral("If the daily limit is always negative, what should you do?"), {QStringLiteral("Review the plan or expenses"), QStringLiteral("Delete history"), QStringLiteral("Always increase risk")}, 0},
                    {QStringLiteral("The main idea of a daily budget is:"), {QStringLiteral("See the spending boundary and react quickly"), QStringLiteral("Ban every purchase"), QStringLiteral("Track only income")}, 0}
                }
            },
            {
                QStringLiteral("How Bonds Work"),
                QStringLiteral("Coupon, face value, maturity, and key risks"),
                QStringLiteral("<h2>How bonds work</h2><p>A bond is a debt security. The buyer lends money to the issuer: a government, region, or company. In return, the issuer usually pays coupons.</p><p>A bond has a face value, coupon, and maturity date. Face value is the amount usually returned at maturity. Coupon is a regular interest payment. Market price can be above or below face value.</p><p>Bond yield depends on purchase price, coupon size, and time to maturity. When market rates rise, older low-coupon bonds often fall in price. When rates fall, they may rise.</p><p>Main risks are issuer default, interest-rate changes, and low liquidity. Government bonds are usually safer than corporate bonds, but often yield less.</p>"),
                {
                    {QStringLiteral("What is a bond?"), {QStringLiteral("A debt security"), QStringLiteral("A company share"), QStringLiteral("A crypto token")}, 0},
                    {QStringLiteral("Who is an issuer?"), {QStringLiteral("The party that issued the bond and borrowed money"), QStringLiteral("A shop customer"), QStringLiteral("A trading bot")}, 0},
                    {QStringLiteral("What is a coupon?"), {QStringLiteral("A regular interest payment"), QStringLiteral("A cafe discount"), QStringLiteral("A broker fee")}, 0},
                    {QStringLiteral("What is face value?"), {QStringLiteral("The amount usually returned at maturity"), QStringLiteral("A stock price"), QStringLiteral("Tax amount")}, 0},
                    {QStringLiteral("What happens at maturity?"), {QStringLiteral("The issuer returns face value"), QStringLiteral("The bond becomes a stock"), QStringLiteral("The coupon becomes infinite")}, 0},
                    {QStringLiteral("Why can a bond price change?"), {QStringLiteral("Rates, risk, and demand"), QStringLiteral("Only because of app color"), QStringLiteral("It is always fixed")}, 0},
                    {QStringLiteral("What often happens to old bonds when rates rise?"), {QStringLiteral("They may fall in price"), QStringLiteral("They always rise"), QStringLiteral("They disappear")}, 0},
                    {QStringLiteral("What is default risk?"), {QStringLiteral("Issuer may fail to pay"), QStringLiteral("Broker changes theme"), QStringLiteral("Investor forgets password")}, 0},
                    {QStringLiteral("Which bonds are usually considered safer?"), {QStringLiteral("Government bonds"), QStringLiteral("Any bond with the highest coupon"), QStringLiteral("Only foreign bonds")}, 0},
                    {QStringLiteral("Bond yield depends on:"), {QStringLiteral("Purchase price, coupons, and term"), QStringLiteral("Only the name"), QStringLiteral("Number of app tabs")}, 0}
                }
            },
            {
                QStringLiteral("Leverage and Futures"),
                QStringLiteral("Margin, collateral, built-in leverage, and liquidation risk"),
                QStringLiteral("<h2>Leverage and futures</h2><p>Leverage means opening a position larger than your own capital. The broker or exchange lets you control more money, but losses are also calculated from the full position.</p><p>Leverage speeds up both profit and loss. With 5:1 leverage, a 10% move against the position can mean about a 50% loss of your own capital before fees and exchange rules.</p><p>A future is a contract for future delivery or cash settlement based on an underlying asset. Private investors usually trade cash-settled futures.</p><p>Futures require initial margin. It is collateral, not the full contract price. This creates built-in leverage. If losses become too large, the broker can require more funds or close the position.</p>"),
                {
                    {QStringLiteral("What does leverage do?"), {QStringLiteral("Increases position size relative to own capital"), QStringLiteral("Guarantees profit"), QStringLiteral("Removes risk")}, 0},
                    {QStringLiteral("How does leverage affect losses?"), {QStringLiteral("It speeds up losses like profits"), QStringLiteral("It removes losses"), QStringLiteral("It makes losses impossible")}, 0},
                    {QStringLiteral("What is a future?"), {QStringLiteral("A contract for future delivery or settlement"), QStringLiteral("A bank deposit"), QStringLiteral("A type of bond")}, 0},
                    {QStringLiteral("What is initial margin?"), {QStringLiteral("Collateral for a futures position"), QStringLiteral("Full apartment price"), QStringLiteral("Bond coupon")}, 0},
                    {QStringLiteral("Why do futures have built-in leverage?"), {QStringLiteral("You post collateral, not full contract value"), QStringLiteral("Exchange gives free money"), QStringLiteral("Price never changes")}, 0},
                    {QStringLiteral("What can a broker do after a large loss?"), {QStringLiteral("Ask for more funds or close the position"), QStringLiteral("Forgive the loss"), QStringLiteral("Issue a certificate")}, 0},
                    {QStringLiteral("With 5:1 leverage and a 10% move against you, the own-capital loss is roughly:"), {QStringLiteral("About 50%"), QStringLiteral("About 2%"), QStringLiteral("Zero")}, 0},
                    {QStringLiteral("Who should be especially careful with futures and leverage?"), {QStringLiteral("Beginners without risk management"), QStringLiteral("Only banks"), QStringLiteral("People who track expenses")}, 0},
                    {QStringLiteral("Before a leveraged trade, you should:"), {QStringLiteral("Define risk and exit scenario"), QStringLiteral("Ignore position size"), QStringLiteral("Use maximum leverage")}, 0},
                    {QStringLiteral("Main takeaway about leverage:"), {QStringLiteral("It is a high-risk tool"), QStringLiteral("It is an emergency fund"), QStringLiteral("It removes volatility")}, 0}
                }
            }
        };
    }

    return {
        {
            QStringLiteral("Как откладывать"),
            QStringLiteral("Подушка безопасности, регулярность и простой дневной бюджет"),
            QStringLiteral("<h2>Как откладывать деньги</h2><p>Откладывание начинается не с крупной зарплаты, а с правила. Сначала зафиксируй сумму или процент, который уходят в накопления сразу после дохода. Это принцип 'сначала заплати себе'.</p><p>База личных финансов - подушка безопасности. Обычно ее считают в месяцах обязательных расходов: еда, транспорт, жилье, связь, лечение. Хорошая цель для старта - 3 месяца расходов.</p><p>Дневной лимит помогает держать расход под контролем. Если лимит 500 рублей, а сегодня потрачено 540, остаток становится -40. Это не ошибка, а сигнал: завтра лучше потратить меньше или пересмотреть план.</p><p>Накопления лучше разделять по целям: подушка, крупная покупка, инвестиции. Деньги на ближайшие траты не стоит держать в рискованных активах.</p>"),
            {
                {QStringLiteral("Что означает правило 'сначала заплати себе'?"), {QStringLiteral("Сначала отложить деньги, потом тратить"), QStringLiteral("Сначала оплатить кредиты друзей"), QStringLiteral("Потратить все и отложить остаток")}, 0},
                {QStringLiteral("Для чего нужна подушка безопасности?"), {QStringLiteral("Для случайных обязательных расходов и потери дохода"), QStringLiteral("Для торговли с плечом"), QStringLiteral("Для покупки самых рискованных активов")}, 0},
                {QStringLiteral("Как обычно измеряют размер подушки?"), {QStringLiteral("В месяцах обязательных расходов"), QStringLiteral("В количестве банковских карт"), QStringLiteral("В процентах от цены акций")}, 0},
                {QStringLiteral("Что показывает отрицательный остаток дневного лимита?"), {QStringLiteral("За день потрачено больше плана"), QStringLiteral("Доход вырос"), QStringLiteral("Лимит стал бесконечным")}, 0},
                {QStringLiteral("Где лучше держать деньги на ближайшие обязательные траты?"), {QStringLiteral("В надежном и ликвидном месте"), QStringLiteral("Только во фьючерсах"), QStringLiteral("В случайной криптовалюте")}, 0},
                {QStringLiteral("Почему маленькая регулярная сумма полезна?"), {QStringLiteral("Она создает привычку и снижает нагрузку"), QStringLiteral("Она гарантирует сверхдоходность"), QStringLiteral("Она отменяет учет расходов")}, 0},
                {QStringLiteral("Что лучше сделать с накоплениями?"), {QStringLiteral("Разделить по целям"), QStringLiteral("Смешать все в одну непонятную сумму"), QStringLiteral("Всегда держать наличными под рукой")}, 0},
                {QStringLiteral("Что входит в обязательные расходы?"), {QStringLiteral("Еда, жилье, транспорт, связь"), QStringLiteral("Любые развлечения"), QStringLiteral("Только инвестиции")}, 0},
                {QStringLiteral("Если лимит постоянно уходит в минус, что стоит сделать?"), {QStringLiteral("Пересмотреть план или расходы"), QStringLiteral("Удалить историю расходов"), QStringLiteral("Всегда повышать риск")}, 0},
                {QStringLiteral("Главная идея бюджета на день:"), {QStringLiteral("Видеть границу трат и быстро реагировать"), QStringLiteral("Запретить любые покупки"), QStringLiteral("Считать только доходы")}, 0}
            }
        },
        {
            QStringLiteral("Как работают облигации"),
            QStringLiteral("Купон, номинал, срок погашения и главные риски"),
            QStringLiteral("<h2>Как работают облигации</h2><p>Облигация - это долговая бумага. Покупатель облигации фактически дает деньги в долг эмитенту: государству, региону или компании. За это эмитент обычно платит купоны.</p><p>У облигации есть номинал, купон и дата погашения. Номинал - сумма, которую должны вернуть при погашении. Купон - регулярный процентный платеж. Рыночная цена может быть выше или ниже номинала.</p><p>Доходность облигации зависит от цены покупки, размера купонов и срока до погашения. Если ставка в экономике растет, старые облигации с низким купоном часто дешевеют. Если ставка падает, они могут дорожать.</p><p>Главные риски: дефолт эмитента, изменение процентных ставок, низкая ликвидность. Государственные облигации обычно надежнее корпоративных, но доходность у них часто ниже.</p>"),
            {
                {QStringLiteral("Что такое облигация?"), {QStringLiteral("Долговая бумага"), QStringLiteral("Доля в компании"), QStringLiteral("Криптовалютный токен")}, 0},
                {QStringLiteral("Кто такой эмитент?"), {QStringLiteral("Тот, кто выпустил облигацию и занял деньги"), QStringLiteral("Покупатель в магазине"), QStringLiteral("Биржевой робот")}, 0},
                {QStringLiteral("Что такое купон?"), {QStringLiteral("Регулярный процентный платеж по облигации"), QStringLiteral("Скидка в кафе"), QStringLiteral("Комиссия брокера")}, 0},
                {QStringLiteral("Что такое номинал?"), {QStringLiteral("Сумма, которую обычно возвращают при погашении"), QStringLiteral("Цена акции"), QStringLiteral("Размер налога")}, 0},
                {QStringLiteral("Что происходит в дату погашения?"), {QStringLiteral("Эмитент возвращает номинал"), QStringLiteral("Облигация превращается в акцию"), QStringLiteral("Купон становится бесконечным")}, 0},
                {QStringLiteral("Почему цена облигации может меняться?"), {QStringLiteral("Из-за ставок, риска и спроса"), QStringLiteral("Только из-за цвета приложения"), QStringLiteral("Она всегда фиксирована")}, 0},
                {QStringLiteral("Что обычно происходит со старыми облигациями при росте ставок?"), {QStringLiteral("Они могут дешеветь"), QStringLiteral("Они всегда дорожают"), QStringLiteral("Они исчезают")}, 0},
                {QStringLiteral("Что такое риск дефолта?"), {QStringLiteral("Эмитент может не выплатить долг"), QStringLiteral("Брокер поменяет тему интерфейса"), QStringLiteral("Инвестор забудет пароль")}, 0},
                {QStringLiteral("Какие облигации обычно считаются надежнее?"), {QStringLiteral("Государственные"), QStringLiteral("Любые с самым высоким купоном"), QStringLiteral("Только иностранные")}, 0},
                {QStringLiteral("Доходность облигации зависит от:"), {QStringLiteral("Цены покупки, купонов и срока"), QStringLiteral("Только названия бумаги"), QStringLiteral("Количества вкладок в приложении")}, 0}
            }
        },
        {
            QStringLiteral("Плечо и фьючерсы"),
            QStringLiteral("Маржинальная торговля, гарантийное обеспечение и риск ликвидации"),
            QStringLiteral("<h2>Что такое торговля с плечом и фьючерсы</h2><p>Торговля с плечом означает, что инвестор открывает позицию больше, чем его собственные деньги. Брокер или биржа дает возможность управлять большей суммой, но убытки тоже считаются от полной позиции.</p><p>Плечо ускоряет и прибыль, и убыток. Если позиция с плечом 5 к 1 меняется против инвестора на 10%, потеря по собственному капиталу может быть около 50% до комиссий и правил биржи.</p><p>Фьючерс - это контракт на будущую поставку или расчет по цене базового актива. В реальности частный инвестор чаще торгует расчетными фьючерсами: результат пересчитывается деньгами.</p><p>Для фьючерса требуется гарантийное обеспечение. Это не цена всего контракта, а залог. Из-за этого возникает встроенное плечо. Если убыток становится слишком большим, брокер может потребовать пополнить счет или закрыть позицию.</p>"),
            {
                {QStringLiteral("Что делает плечо?"), {QStringLiteral("Увеличивает размер позиции относительно собственных денег"), QStringLiteral("Гарантирует прибыль"), QStringLiteral("Отменяет риск")}, 0},
                {QStringLiteral("Как плечо влияет на убытки?"), {QStringLiteral("Ускоряет убытки так же, как прибыль"), QStringLiteral("Полностью убирает убытки"), QStringLiteral("Делает убытки невозможными")}, 0},
                {QStringLiteral("Что такое фьючерс?"), {QStringLiteral("Контракт на будущий расчет или поставку"), QStringLiteral("Обычный банковский вклад"), QStringLiteral("Тип облигации")}, 0},
                {QStringLiteral("Что такое гарантийное обеспечение?"), {QStringLiteral("Залог для открытия фьючерсной позиции"), QStringLiteral("Полная цена квартиры"), QStringLiteral("Купон облигации")}, 0},
                {QStringLiteral("Почему во фьючерсах есть встроенное плечо?"), {QStringLiteral("Потому что вносится залог, а не вся стоимость контракта"), QStringLiteral("Потому что биржа дарит деньги"), QStringLiteral("Потому что цена не меняется")}, 0},
                {QStringLiteral("Что может сделать брокер при большом убытке?"), {QStringLiteral("Потребовать пополнить счет или закрыть позицию"), QStringLiteral("Простить убыток"), QStringLiteral("Выдать сертификат")}, 0},
                {QStringLiteral("Плечо 5 к 1 и движение против позиции на 10% примерно означает:"), {QStringLiteral("Около 50% убытка на собственный капитал"), QStringLiteral("Около 2% убытка"), QStringLiteral("Нулевой риск")}, 0},
                {QStringLiteral("Кому особенно опасны фьючерсы и плечо?"), {QStringLiteral("Новичкам без риск-менеджмента"), QStringLiteral("Только банкам"), QStringLiteral("Тем, кто ведет учет расходов")}, 0},
                {QStringLiteral("Что важно сделать перед сделкой с плечом?"), {QStringLiteral("Заранее определить риск и стоп-сценарий"), QStringLiteral("Игнорировать размер позиции"), QStringLiteral("Взять максимальное плечо")}, 0},
                {QStringLiteral("Главный вывод по плечу:"), {QStringLiteral("Это инструмент повышенного риска"), QStringLiteral("Это аналог подушки безопасности"), QStringLiteral("Это способ убрать волатильность")}, 0}
            }
        }
    };
}

QString MainWindow::t(const QString& ru, const QString& en) const
{
    return m_language == UiLanguage::English ? en : ru;
}

QString MainWindow::displayCategory(const QString& category) const
{
    if (m_language != UiLanguage::English) {
        return category;
    }
    if (category == QStringLiteral("Еда")) return QStringLiteral("Food");
    if (category == QStringLiteral("Транспорт")) return QStringLiteral("Transport");
    if (category == QStringLiteral("Кафе")) return QStringLiteral("Cafe");
    if (category == QStringLiteral("Развлечения")) return QStringLiteral("Entertainment");
    if (category == QStringLiteral("Здоровье")) return QStringLiteral("Health");
    if (category == QStringLiteral("Учеба")) return QStringLiteral("Education");
    if (category == QStringLiteral("Жилье")) return QStringLiteral("Housing");
    if (category == QStringLiteral("Подписки")) return QStringLiteral("Subscriptions");
    if (category == QStringLiteral("Подарки")) return QStringLiteral("Gifts");
    if (category == QStringLiteral("Прочее")) return QStringLiteral("Other");
    return category;
}

QString MainWindow::displayAssetType(const QString& type) const
{
    if (m_language != UiLanguage::English) {
        return type;
    }
    if (type == QStringLiteral("Акции")) return QStringLiteral("Stocks");
    if (type == QStringLiteral("Облигации")) return QStringLiteral("Bonds");
    if (type == QStringLiteral("Вклад")) return QStringLiteral("Deposit");
    if (type == QStringLiteral("Крипта")) return QStringLiteral("Crypto");
    if (type == QStringLiteral("Монеты")) return QStringLiteral("Coins");
    if (type == QStringLiteral("Наличные")) return QStringLiteral("Cash");
    return type;
}

QString MainWindow::formatMoney(double value) const
{
    return QLocale(QLocale::Russian, QLocale::Russia).toString(value, 'f', 2) + " RUB";
}
