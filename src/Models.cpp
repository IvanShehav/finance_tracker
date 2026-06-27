#include "Models.h"

#include <QJsonValue>

QJsonObject Expense::toJson() const
{
    return {
        {"date", date.toString(Qt::ISODate)},
        {"category", category},
        {"title", title},
        {"amount", amount}
    };
}

Expense Expense::fromJson(const QJsonObject& object)
{
    Expense expense;
    expense.date = QDate::fromString(object.value("date").toString(), Qt::ISODate);
    if (!expense.date.isValid()) {
        expense.date = QDate::currentDate();
    }
    expense.category = object.value("category").toString();
    expense.title = object.value("title").toString();
    expense.amount = object.value("amount").toDouble();
    return expense;
}

double CategoryBudget::remaining() const
{
    return limit - spent;
}

QJsonObject CategoryBudget::toJson() const
{
    return {
        {"category", category},
        {"limit", limit}
    };
}

CategoryBudget CategoryBudget::fromJson(const QJsonObject& object)
{
    CategoryBudget budget;
    budget.category = object.value("category").toString();
    budget.limit = object.value("limit").toDouble();
    return budget;
}

double Asset::marketValue() const
{
    return quantity * price;
}

QJsonObject Asset::toJson() const
{
    return {
        {"type", type},
        {"name", name},
        {"quantity", quantity},
        {"price", price}
    };
}

Asset Asset::fromJson(const QJsonObject& object)
{
    Asset asset;
    asset.type = object.value("type").toString();
    asset.name = object.value("name").toString();
    asset.quantity = object.value("quantity").toDouble();
    asset.price = object.value("price").toDouble();
    return asset;
}

QStringList defaultBudgetCategories()
{
    return {
        QStringLiteral("Еда"),
        QStringLiteral("Транспорт"),
        QStringLiteral("Кафе"),
        QStringLiteral("Развлечения"),
        QStringLiteral("Здоровье"),
        QStringLiteral("Учеба"),
        QStringLiteral("Жилье"),
        QStringLiteral("Подписки"),
        QStringLiteral("Подарки"),
        QStringLiteral("Прочее")
    };
}

QStringList defaultAssetTypes()
{
    return {
        QStringLiteral("Акции"),
        QStringLiteral("Облигации"),
        QStringLiteral("Вклад"),
        QStringLiteral("Крипта"),
        QStringLiteral("Монеты"),
        QStringLiteral("ETF"),
        QStringLiteral("Наличные")
    };
}
