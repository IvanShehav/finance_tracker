#pragma once

#include "ChartWidgets.h"
#include "FinanceManager.h"
#include "InsiderService.h"
#include "MarketService.h"

#include <QButtonGroup>
#include <QMainWindow>
#include <QVector>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QTableWidget;
class QTabWidget;
class QTextEdit;

struct QuizQuestion {
    QString text;
    QStringList answers;
    int correctIndex = 0;
};

struct Course {
    QString title;
    QString subtitle;
    QString theory;
    QVector<QuizQuestion> questions;
};

enum class UiLanguage {
    Russian,
    English
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void toggleTheme();
    void toggleLanguage();
    void saveDailyLimit();
    void addExpense();
    void editSelectedExpense();
    void deleteSelectedExpense();
    void cancelExpenseEdit();
    void addAsset();
    void editSelectedAsset();
    void deleteSelectedAsset();
    void cancelAssetEdit();
    void refreshInsiderTrades();
    void showInsiderTrades(const QVector<InsiderTrade>& trades, const QString& message, bool usedFallback);
    void loadMarketData();
    void showMarketData(const QString& title, const QVector<MarketPoint>& points, const QString& message, bool usedFallback);
    void selectCourse(int row);
    void submitQuiz();
    void refreshViews();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void rebuildUi();
    QWidget* buildTodayTab();
    QWidget* buildReportsTab();
    QWidget* buildAssetsTab();
    QWidget* buildMarketsTab();
    QWidget* buildInsiderTab();
    QWidget* buildEducationTab();

    void applyTheme();
    void updateCategoryCombos();
    void refreshTodayPanel();
    void refreshExpenseTable();
    void refreshReportTable();
    void refreshAssetTable();
    void startExpenseEdit(int index);
    void resetExpenseEditor();
    void setExpenseCategory(const QString& category);
    void startAssetEdit(int index);
    void resetAssetEditor();
    void showCalendarPopup(QDateEdit* target);
    void renderCourse(int index);
    QVector<Course> createCourses() const;
    QString t(const QString& ru, const QString& en) const;
    QString displayCategory(const QString& category) const;
    QString displayAssetType(const QString& type) const;
    QString formatMoney(double value) const;

    FinanceManager m_manager;
    InsiderService m_insiderService;
    MarketService m_marketService;
    QVector<Course> m_courses;
    int m_currentCourseIndex = 0;
    UiLanguage m_language = UiLanguage::Russian;
    bool m_darkMode = false;

    QTabWidget* m_tabs = nullptr;
    QPushButton* m_themeButton = nullptr;
    QPushButton* m_languageButton = nullptr;

    QLabel* m_todaySpentLabel = nullptr;
    QLabel* m_todayLimitLabel = nullptr;
    QLabel* m_todayRemainingLabel = nullptr;
    QLabel* m_todayStatusLabel = nullptr;
    QDoubleSpinBox* m_dailyLimitSpin = nullptr;
    QLineEdit* m_expenseTitleEdit = nullptr;
    QDateEdit* m_expenseDateEdit = nullptr;
    QButtonGroup* m_expenseCategoryGroup = nullptr;
    QWidget* m_expenseCategoryPanel = nullptr;
    QString m_selectedExpenseCategory;
    QDoubleSpinBox* m_expenseAmountSpin = nullptr;
    QPushButton* m_addExpenseButton = nullptr;
    QPushButton* m_cancelExpenseEditButton = nullptr;
    QPushButton* m_editExpenseButton = nullptr;
    QPushButton* m_deleteExpenseButton = nullptr;
    QTableWidget* m_todayExpenseTable = nullptr;
    QVector<int> m_todayExpenseIndexes;
    int m_editingExpenseIndex = -1;

    QDateEdit* m_reportFromDateEdit = nullptr;
    QDateEdit* m_reportToDateEdit = nullptr;
    QLabel* m_reportSummaryLabel = nullptr;
    QTableWidget* m_reportTable = nullptr;
    QTableWidget* m_reportExpenseTable = nullptr;
    QVector<int> m_reportExpenseIndexes;
    BudgetChartWidget* m_budgetChart = nullptr;

    QLabel* m_assetsSummaryLabel = nullptr;
    QComboBox* m_assetTypeBox = nullptr;
    QLineEdit* m_assetNameEdit = nullptr;
    QDoubleSpinBox* m_assetQuantitySpin = nullptr;
    QDoubleSpinBox* m_assetPriceSpin = nullptr;
    QPushButton* m_addAssetButton = nullptr;
    QPushButton* m_cancelAssetEditButton = nullptr;
    QPushButton* m_editAssetButton = nullptr;
    QPushButton* m_deleteAssetButton = nullptr;
    QTableWidget* m_assetTable = nullptr;
    AssetChartWidget* m_assetChart = nullptr;
    QVector<int> m_assetIndexes;
    int m_editingAssetIndex = -1;

    QString m_marketMode = QStringLiteral("stock");
    QButtonGroup* m_marketModeGroup = nullptr;
    QLineEdit* m_marketSymbolEdit = nullptr;
    QPushButton* m_marketLoadButton = nullptr;
    QLabel* m_marketStatusLabel = nullptr;
    MarketChartWidget* m_marketChart = nullptr;

    QLabel* m_insiderStatusLabel = nullptr;
    QPushButton* m_loadInsidersButton = nullptr;
    QTableWidget* m_insiderTable = nullptr;

    QListWidget* m_courseList = nullptr;
    QLabel* m_courseTitleLabel = nullptr;
    QLabel* m_courseSubtitleLabel = nullptr;
    QTextEdit* m_courseTheoryText = nullptr;
    QVector<QButtonGroup*> m_quizGroups;
    QVector<QLabel*> m_quizQuestionLabels;
    QVector<QVector<QRadioButton*>> m_quizButtons;
    QLabel* m_quizResultLabel = nullptr;
    QPushButton* m_submitQuizButton = nullptr;
};
