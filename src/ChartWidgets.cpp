#include "ChartWidgets.h"

#include <QFontMetrics>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {

QString moneyShort(double value)
{
    if (qAbs(value) >= 1000000.0) {
        return QString::number(value / 1000000.0, 'f', 1) + "M";
    }
    if (qAbs(value) >= 1000.0) {
        return QString::number(value / 1000.0, 'f', 1) + "K";
    }
    return QString::number(value, 'f', 0);
}

QVector<QColor> chartColors()
{
    return {
        QColor("#2563eb"),
        QColor("#10b981"),
        QColor("#f59e0b"),
        QColor("#ef4444"),
        QColor("#8b5cf6"),
        QColor("#06b6d4"),
        QColor("#64748b"),
        QColor("#ec4899")
    };
}

} // namespace

BudgetChartWidget::BudgetChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(280);
}

void BudgetChartWidget::setBudgets(const QVector<CategoryBudget>& budgets)
{
    m_budgets = budgets;
    update();
}

void BudgetChartWidget::setTexts(const QString& title, const QString& emptyText)
{
    m_title = title;
    m_emptyText = emptyText;
    update();
}

QSize BudgetChartWidget::sizeHint() const
{
    return {420, 300};
}

void BudgetChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#f8fafc"));

    const QRect area = rect().adjusted(22, 22, -22, -22);
    painter.setPen(QColor("#111827"));
    painter.setFont(QFont(font().family(), 12, QFont::DemiBold));
    painter.drawText(area.left(), area.top(), m_title);

    double maxValue = 1.0;
    int visibleRows = 0;
    for (const CategoryBudget& budget : m_budgets) {
        if (budget.spent > 0.0) {
            maxValue = qMax(maxValue, budget.spent);
            ++visibleRows;
        }
    }

    if (visibleRows == 0) {
        painter.setPen(QColor("#64748b"));
        painter.setFont(QFont(font().family(), 10));
        painter.drawText(area, Qt::AlignCenter, m_emptyText);
        return;
    }

    const int top = area.top() + 42;
    const int rowHeight = qMax(30, (area.height() - 44) / qMax(1, visibleRows));
    const int labelWidth = 122;
    const int barLeft = area.left() + labelWidth;
    const int barMaxWidth = qMax(80, area.width() - labelWidth - 88);
    const QFontMetrics metrics(font());
    const QVector<QColor> colors = chartColors();

    painter.setFont(QFont(font().family(), 9));
    int row = 0;
    for (int i = 0; i < m_budgets.size(); ++i) {
        const CategoryBudget& budget = m_budgets.at(i);
        if (budget.spent <= 0.0) {
            continue;
        }

        const int y = top + row * rowHeight;
        const QRect track(barLeft, y + 7, barMaxWidth, 16);
        const int spentWidth = qRound((budget.spent / maxValue) * barMaxWidth);
        const QColor fill = colors.at(i % colors.size());

        painter.setPen(QColor("#334155"));
        painter.drawText(area.left(), y + 20, metrics.elidedText(budget.category, Qt::ElideRight, labelWidth - 10));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#e2e8f0"));
        painter.drawRoundedRect(track, 8, 8);

        painter.setBrush(fill);
        painter.drawRoundedRect(QRect(track.left(), track.top(), qMax(4, spentWidth), track.height()), 8, 8);

        painter.setPen(QColor("#475569"));
        painter.drawText(track.right() + 10, y + 20, moneyShort(budget.spent));
        ++row;
    }
}

AssetChartWidget::AssetChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(280);
}

void AssetChartWidget::setAssets(const QVector<Asset>& assets)
{
    m_assets = assets;
    update();
}

void AssetChartWidget::setTexts(const QString& title, const QString& emptyText)
{
    m_title = title;
    m_emptyText = emptyText;
    update();
}

QSize AssetChartWidget::sizeHint() const
{
    return {420, 300};
}

void AssetChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#f8fafc"));

    const QRect area = rect().adjusted(22, 22, -22, -22);
    painter.setPen(QColor("#111827"));
    painter.setFont(QFont(font().family(), 12, QFont::DemiBold));
    painter.drawText(area.left(), area.top(), m_title);

    QMap<QString, double> totalsByType;
    double total = 0.0;
    for (const Asset& asset : m_assets) {
        const double value = asset.marketValue();
        totalsByType[asset.type] += value;
        total += value;
    }

    if (total <= 0.0) {
        painter.setPen(QColor("#64748b"));
        painter.setFont(QFont(font().family(), 10));
        painter.drawText(area, Qt::AlignCenter, m_emptyText);
        return;
    }

    const int chartSize = qMin(area.width() / 2, area.height() - 42);
    const QRect pieRect(area.left(), area.top() + 46, chartSize, chartSize);
    const QVector<QColor> colors = chartColors();

    int startAngle = 90 * 16;
    int colorIndex = 0;
    for (auto it = totalsByType.cbegin(); it != totalsByType.cend(); ++it) {
        const int spanAngle = qRound((it.value() / total) * 360.0 * 16.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.at(colorIndex % colors.size()));
        painter.drawPie(pieRect, startAngle, -spanAngle);
        startAngle -= spanAngle;
        ++colorIndex;
    }

    const int legendLeft = pieRect.right() + 24;
    int y = area.top() + 54;
    colorIndex = 0;
    painter.setFont(QFont(font().family(), 9));
    for (auto it = totalsByType.cbegin(); it != totalsByType.cend(); ++it) {
        const QColor color = colors.at(colorIndex % colors.size());
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawRoundedRect(QRect(legendLeft, y - 11, 13, 13), 4, 4);

        painter.setPen(QColor("#334155"));
        const double percent = it.value() / total * 100.0;
        painter.drawText(legendLeft + 22, y, QString("%1: %2%").arg(it.key()).arg(percent, 0, 'f', 1));
        y += 25;
        ++colorIndex;
    }
}

MarketChartWidget::MarketChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(360);
}

void MarketChartWidget::setSeries(const QString& title, const QVector<MarketPoint>& points, const QString& currency)
{
    m_title = title;
    m_points = points;
    m_currency = currency;
    update();
}

void MarketChartWidget::setEmptyText(const QString& emptyText)
{
    m_emptyText = emptyText;
    update();
}

QSize MarketChartWidget::sizeHint() const
{
    return {720, 380};
}

void MarketChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#f8fafc"));

    const QRect area = rect().adjusted(24, 22, -24, -24);
    painter.setPen(QColor("#111827"));
    painter.setFont(QFont(font().family(), 13, QFont::DemiBold));
    painter.drawText(area.left(), area.top(), m_title);

    if (m_points.size() < 2) {
        painter.setPen(QColor("#64748b"));
        painter.setFont(QFont(font().family(), 10));
        painter.drawText(area, Qt::AlignCenter, m_emptyText);
        return;
    }

    double minPrice = m_points.first().price;
    double maxPrice = m_points.first().price;
    for (const MarketPoint& point : m_points) {
        minPrice = qMin(minPrice, point.price);
        maxPrice = qMax(maxPrice, point.price);
    }
    if (qFuzzyCompare(minPrice, maxPrice)) {
        minPrice *= 0.98;
        maxPrice *= 1.02;
    }

    const QRect chart = area.adjusted(8, 58, -16, -38);
    painter.setPen(QColor("#d9e2ef"));
    for (int i = 0; i <= 4; ++i) {
        const int y = chart.top() + i * chart.height() / 4;
        painter.drawLine(chart.left(), y, chart.right(), y);
    }

    auto pointToScreen = [&](int index) {
        const double xRatio = index / qMax(1.0, static_cast<double>(m_points.size() - 1));
        const double yRatio = (m_points.at(index).price - minPrice) / (maxPrice - minPrice);
        return QPointF(chart.left() + xRatio * chart.width(), chart.bottom() - yRatio * chart.height());
    };

    QPainterPath path;
    path.moveTo(pointToScreen(0));
    for (int i = 1; i < m_points.size(); ++i) {
        path.lineTo(pointToScreen(i));
    }

    const QColor lineColor("#2563eb");
    painter.setPen(QPen(lineColor, 3));
    painter.drawPath(path);

    const QPointF last = pointToScreen(m_points.size() - 1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#2563eb"));
    painter.drawEllipse(last, 5, 5);

    painter.setFont(QFont(font().family(), 9));
    painter.setPen(QColor("#475569"));
    painter.drawText(chart.left(), chart.bottom() + 24, m_points.first().date.toString("dd.MM"));
    painter.drawText(chart.right() - 54, chart.bottom() + 24, m_points.last().date.toString("dd.MM"));

    painter.drawText(chart.right() - 90, chart.top() - 8, QString::number(maxPrice, 'f', 2));
    painter.drawText(chart.right() - 90, chart.bottom() + 4, QString::number(minPrice, 'f', 2));

    painter.setPen(QColor("#111827"));
    painter.setFont(QFont(font().family(), 12, QFont::DemiBold));
    painter.drawText(
        area.left(),
        area.top() + 34,
        QString("%1 %2").arg(m_points.last().price, 0, 'f', 2).arg(m_currency)
    );
}
