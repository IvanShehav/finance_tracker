#pragma once

#include "Models.h"

#include <QObject>
#include <QVector>

class FinanceManager : public QObject {
    Q_OBJECT

public:
    explicit FinanceManager(QObject* parent = nullptr);

    const QVector<CategoryBudget>& budgets() const;
    const QVector<Expense>& expenses() const;
    const QVector<Asset>& assets() const;

    void load();
    bool save() const;

    double dailyLimit() const;
    void setDailyLimit(double limit);
    void addExpense(const Expense& expense);
    bool updateExpense(int index, const Expense& expense);
    bool removeExpense(int index);
    void addAsset(const Asset& asset);
    bool updateAsset(int index, const Asset& asset);
    bool removeAsset(int index);

    double totalLimit() const;
    double totalSpent() const;
    double totalRemaining() const;
    double spentOnDate(const QDate& date) const;
    double remainingOnDate(const QDate& date) const;
    double totalAssetsValue() const;

signals:
    void changed();

private:
    QString storagePath() const;
    void ensureDefaultBudgets();
    void seedDemoData();
    void rebuildSpent();
    CategoryBudget* budgetForCategory(const QString& category);

    double m_dailyLimit = 500.0;
    QVector<CategoryBudget> m_budgets;
    QVector<Expense> m_expenses;
    QVector<Asset> m_assets;
};
