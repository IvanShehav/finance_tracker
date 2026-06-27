#include "FinanceManager.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

FinanceManager::FinanceManager(QObject* parent)
    : QObject(parent)
{
}

const QVector<CategoryBudget>& FinanceManager::budgets() const
{
    return m_budgets;
}

const QVector<Expense>& FinanceManager::expenses() const
{
    return m_expenses;
}

const QVector<Asset>& FinanceManager::assets() const
{
    return m_assets;
}

void FinanceManager::load()
{
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        ensureDefaultBudgets();
        seedDemoData();
        rebuildSpent();
        save();
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject root = document.object();
    const int demoVersion = root.value("demoVersion").toInt(0);
    m_dailyLimit = root.value("dailyLimit").toDouble(500.0);

    m_budgets.clear();
    for (const QJsonValue& value : root.value("budgets").toArray()) {
        const CategoryBudget budget = CategoryBudget::fromJson(value.toObject());
        if (!budget.category.isEmpty()) {
            m_budgets.push_back(budget);
        }
    }

    m_expenses.clear();
    for (const QJsonValue& value : root.value("expenses").toArray()) {
        m_expenses.push_back(Expense::fromJson(value.toObject()));
    }

    m_assets.clear();
    for (const QJsonValue& value : root.value("assets").toArray()) {
        m_assets.push_back(Asset::fromJson(value.toObject()));
    }

    if (demoVersion < 2) {
        m_budgets.clear();
        ensureDefaultBudgets();
        seedDemoData();
    } else {
        ensureDefaultBudgets();
    }
    if (m_expenses.isEmpty() && m_assets.isEmpty()) {
        seedDemoData();
    }
    rebuildSpent();
    save();
}

bool FinanceManager::save() const
{
    QJsonArray budgets;
    for (const CategoryBudget& budget : m_budgets) {
        budgets.append(budget.toJson());
    }

    QJsonArray expenses;
    for (const Expense& expense : m_expenses) {
        expenses.append(expense.toJson());
    }

    QJsonArray assets;
    for (const Asset& asset : m_assets) {
        assets.append(asset.toJson());
    }

    QJsonObject root;
    root["demoVersion"] = 2;
    root["dailyLimit"] = m_dailyLimit;
    root["budgets"] = budgets;
    root["expenses"] = expenses;
    root["assets"] = assets;

    QFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

double FinanceManager::dailyLimit() const
{
    return m_dailyLimit;
}

void FinanceManager::setDailyLimit(double limit)
{
    m_dailyLimit = limit;
    save();
    emit changed();
}

void FinanceManager::addExpense(const Expense& expense)
{
    m_expenses.prepend(expense);
    CategoryBudget* budget = budgetForCategory(expense.category);
    if (!budget) {
        m_budgets.push_back({expense.category, 0.0, expense.amount});
    } else {
        budget->spent += expense.amount;
    }

    save();
    emit changed();
}

bool FinanceManager::updateExpense(int index, const Expense& expense)
{
    if (index < 0 || index >= m_expenses.size()) {
        return false;
    }

    m_expenses[index] = expense;
    rebuildSpent();
    save();
    emit changed();
    return true;
}

bool FinanceManager::removeExpense(int index)
{
    if (index < 0 || index >= m_expenses.size()) {
        return false;
    }

    m_expenses.removeAt(index);
    rebuildSpent();
    save();
    emit changed();
    return true;
}

void FinanceManager::addAsset(const Asset& asset)
{
    m_assets.prepend(asset);
    save();
    emit changed();
}

bool FinanceManager::updateAsset(int index, const Asset& asset)
{
    if (index < 0 || index >= m_assets.size()) {
        return false;
    }

    m_assets[index] = asset;
    save();
    emit changed();
    return true;
}

bool FinanceManager::removeAsset(int index)
{
    if (index < 0 || index >= m_assets.size()) {
        return false;
    }

    m_assets.removeAt(index);
    save();
    emit changed();
    return true;
}

double FinanceManager::totalLimit() const
{
    return m_dailyLimit;
}

double FinanceManager::totalSpent() const
{
    return spentOnDate(QDate::currentDate());
}

double FinanceManager::totalRemaining() const
{
    return remainingOnDate(QDate::currentDate());
}

double FinanceManager::spentOnDate(const QDate& date) const
{
    double total = 0.0;
    for (const Expense& expense : m_expenses) {
        if (expense.date == date) {
            total += expense.amount;
        }
    }
    return total;
}

double FinanceManager::remainingOnDate(const QDate& date) const
{
    return m_dailyLimit - spentOnDate(date);
}

double FinanceManager::totalAssetsValue() const
{
    double total = 0.0;
    for (const Asset& asset : m_assets) {
        total += asset.marketValue();
    }
    return total;
}

QString FinanceManager::storagePath() const
{
    return QCoreApplication::applicationDirPath() + "/finance_data.json";
}

void FinanceManager::ensureDefaultBudgets()
{
    for (const QString& category : defaultBudgetCategories()) {
        if (!budgetForCategory(category)) {
            m_budgets.push_back({category, 0.0, 0.0});
        }
    }
}

void FinanceManager::seedDemoData()
{
    m_dailyLimit = 500.0;
    const QDate today = QDate::currentDate();

    m_expenses = {
        {today, QStringLiteral("Кафе"), QStringLiteral("Кофе и сэндвич"), 260.0},
        {today, QStringLiteral("Транспорт"), QStringLiteral("Метро"), 70.0},
        {today, QStringLiteral("Еда"), QStringLiteral("Перекус"), 210.0},
        {today.addDays(-1), QStringLiteral("Еда"), QStringLiteral("Продукты"), 740.0},
        {today.addDays(-2), QStringLiteral("Подписки"), QStringLiteral("Музыка"), 199.0},
        {today.addDays(-3), QStringLiteral("Развлечения"), QStringLiteral("Кино"), 620.0},
        {today.addDays(-5), QStringLiteral("Здоровье"), QStringLiteral("Аптека"), 430.0},
        {today.addDays(-7), QStringLiteral("Транспорт"), QStringLiteral("Такси"), 560.0},
        {today.addDays(-12), QStringLiteral("Учеба"), QStringLiteral("Курс C++"), 1200.0},
        {today.addDays(-18), QStringLiteral("Кафе"), QStringLiteral("Обед"), 390.0},
        {today.addMonths(-1).addDays(-2), QStringLiteral("Жилье"), QStringLiteral("Коммуналка"), 3100.0},
        {today.addMonths(-2).addDays(4), QStringLiteral("Подарки"), QStringLiteral("Подарок другу"), 1800.0},
        {today.addYears(-1).addDays(14), QStringLiteral("Прочее"), QStringLiteral("Старый расход"), 900.0}
    };

    m_assets = {
        {QStringLiteral("Акции"), QStringLiteral("SBER"), 12.0, 315.4},
        {QStringLiteral("Акции"), QStringLiteral("YNDX"), 2.0, 4120.0},
        {QStringLiteral("Облигации"), QStringLiteral("ОФЗ 26243"), 5.0, 987.6},
        {QStringLiteral("Вклад"), QStringLiteral("Накопительный счет"), 1.0, 45000.0},
        {QStringLiteral("Крипта"), QStringLiteral("BTC"), 0.015, 6200000.0},
        {QStringLiteral("Монеты"), QStringLiteral("Золотая монета"), 1.0, 38000.0},
        {QStringLiteral("ETF"), QStringLiteral("TMOS"), 20.0, 8.9}
    };
}

void FinanceManager::rebuildSpent()
{
    for (CategoryBudget& budget : m_budgets) {
        budget.spent = 0.0;
    }

    for (const Expense& expense : m_expenses) {
        CategoryBudget* budget = budgetForCategory(expense.category);
        if (!budget) {
            m_budgets.push_back({expense.category, 0.0, expense.amount});
        } else {
            budget->spent += expense.amount;
        }
    }
}

CategoryBudget* FinanceManager::budgetForCategory(const QString& category)
{
    for (CategoryBudget& budget : m_budgets) {
        if (budget.category == category) {
            return &budget;
        }
    }
    return nullptr;
}
