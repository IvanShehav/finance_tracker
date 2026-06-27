# FinanceTracker Qt

Учебный финансовый трекер на C++ и Qt Widgets.

## Что умеет

- учет дневных расходов по категориям;
- редактирование и удаление расходов и активов;
- лимит по каждой категории, который уменьшается через расходы и может уйти в минус;
- список активов: акции, облигации, вклады, крипта, монеты, ETF, наличные;
- график расходов и круговая диаграмма активов без зависимости от Qt Charts;
- вкладка инсайдерских сделок: загрузка свежих Form 4 из SEC EDGAR через `QNetworkAccessManager` и XML-парсинг;
- вкладка рынков: графики акций через Yahoo Finance API и крипты через CoinGecko API;
- сохранение пользовательских данных в `finance_data.json` рядом с exe-файлом.

## ООП-классы для защиты

- `Expense`, `CategoryBudget`, `Asset`, `InsiderTrade` - модели предметной области.
- `FinanceManager` - хранит коллекции объектов, добавляет расходы/активы, пересчитывает бюджеты, сохраняет JSON.
- `InsiderService` - отдельный сервис для сети и парсинга SEC EDGAR.
- `MarketService` - отдельный сервис для загрузки графиков акций и криптовалют.
- `BudgetChartWidget`, `AssetChartWidget`, `MarketChartWidget` - наследники `QWidget`, переопределяют `paintEvent`.
- `MainWindow` - интерфейс, вкладки, таблицы, сигналы и слоты.

## Как собрать

Открой папку проекта в Qt Creator как CMake-проект и нажми Build/Run.

Если на паре используют старый qmake-подход, можно открыть `FinanceTracker.pro`.

Через терминал:

```powershell
cmake -S . -B build
cmake --build build
```

Если CMake не находит Qt, укажи путь к Qt:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.7.2\mingw_64"
```
