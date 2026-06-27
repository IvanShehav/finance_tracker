#pragma once

#include "Models.h"

#include <QWidget>
#include <QVector>

class BudgetChartWidget : public QWidget {
public:
    explicit BudgetChartWidget(QWidget* parent = nullptr);

    void setBudgets(const QVector<CategoryBudget>& budgets);
    void setTexts(const QString& title, const QString& emptyText);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<CategoryBudget> m_budgets;
    QString m_title = QStringLiteral("Расходы по категориям");
    QString m_emptyText = QStringLiteral("Пока нет расходов за выбранный период");
};

class AssetChartWidget : public QWidget {
public:
    explicit AssetChartWidget(QWidget* parent = nullptr);

    void setAssets(const QVector<Asset>& assets);
    void setTexts(const QString& title, const QString& emptyText);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<Asset> m_assets;
    QString m_title = QStringLiteral("Структура активов");
    QString m_emptyText = QStringLiteral("Добавь активы, чтобы увидеть диаграмму");
};

class MarketChartWidget : public QWidget {
public:
    explicit MarketChartWidget(QWidget* parent = nullptr);

    void setSeries(const QString& title, const QVector<MarketPoint>& points, const QString& currency);
    void setEmptyText(const QString& emptyText);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title = QStringLiteral("График рынка");
    QString m_emptyText = QStringLiteral("Выбери инструмент и нажми загрузить");
    QString m_currency = QStringLiteral("USD");
    QVector<MarketPoint> m_points;
};
