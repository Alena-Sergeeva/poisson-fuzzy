#include "mainwindow.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPen>
#include <QtConcurrent>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <algorithm>
#include <cmath>

// ---------------- Конструктор / UI ----------------
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      watcher(new QFutureWatcher<pf::ProcessResult>(this))
{
    setupUI();
    connect(btnGenerate, &QPushButton::clicked,
            this, &MainWindow::onGenerateClicked);
    connect(watcher, &QFutureWatcher<pf::ProcessResult>::finished,
            this, &MainWindow::onCalculationFinished);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    QWidget*     cw = new QWidget(this);
    QHBoxLayout* ml = new QHBoxLayout(cw);
    QVBoxLayout* ll = new QVBoxLayout();

    // ---- Панель управления ----
    spinCount = new QSpinBox();
    spinCount->setRange(100, 300);
    spinCount->setValue(200);
    spinCount->setPrefix("Кол-во кривых: ");

    spinLMin = new QDoubleSpinBox();
    spinLMin->setRange(0.1, 1000.0);
    spinLMin->setValue(1.0);
    spinLMin->setPrefix("Lambda min: ");

    spinLMax = new QDoubleSpinBox();
    spinLMax->setRange(0.1, 1000.0);
    spinLMax->setValue(100.0);
    spinLMax->setPrefix("Lambda max: ");

    spinThreshold = new QDoubleSpinBox();
    spinThreshold->setRange(0.01, 1.0);
    spinThreshold->setSingleStep(0.05);
    spinThreshold->setDecimals(2);
    spinThreshold->setValue(0.10);
    spinThreshold->setPrefix("Порог (% от макс. PMF): ");

    btnGenerate = new QPushButton("Рассчитать и Построить");
    logText     = new QTextEdit();
    logText->setReadOnly(true);

    ll->addWidget(spinCount);
    ll->addWidget(spinLMin);
    ll->addWidget(spinLMax);
    ll->addWidget(spinThreshold);
    ll->addWidget(btnGenerate);
    ll->addWidget(new QLabel("Лог событий:"));
    ll->addWidget(logText);

    // ---- Вкладка 1: кривые + порог ----
    chartCurves = new QChart();
    chartCurves->setTitle("Пуассон: постоянный порог (красный пунктир). "
                          "Чёрные = ниже порога.");
    chartCurves->legend()->hide();
    viewCurves = new QChartView(chartCurves);
    viewCurves->setRenderHint(QPainter::Antialiasing);

    // ---- Вкладка 2: облако кластеров ----
    chartScatter = new QChart();
    chartScatter->setTitle("Облако кластеров в пространстве (C, R): "
                           "X = центр α-среза, Y = относительная ширина");
    chartScatter->legend()->setAlignment(Qt::AlignBottom);
    chartScatter->legend()->setMarkerShape(QLegend::MarkerShapeCircle);
    viewScatter = new QChartView(chartScatter);
    viewScatter->setRenderHint(QPainter::Antialiasing);

    tabs = new QTabWidget();
    tabs->addTab(viewCurves,  "Кривые и порог");
    tabs->addTab(viewScatter, "Облако кластеров");

    ml->addLayout(ll, 1);
    ml->addWidget(tabs, 3);

    setCentralWidget(cw);
    resize(1300, 750);
}

void MainWindow::logMessage(const QString& msg)
{
    logText->append(QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), msg));
}

// ---------------- Запуск ----------------
void MainWindow::onGenerateClicked()
{
    btnGenerate->setEnabled(false);

    // очистка обеих вкладок
    for (auto* s : chartCurves->series())  { chartCurves->removeSeries(s);  s->deleteLater(); }
    for (auto* a : chartCurves->axes())    { chartCurves->removeAxis(a);    a->deleteLater(); }
    for (auto* s : chartScatter->series()) { chartScatter->removeSeries(s); s->deleteLater(); }
    for (auto* a : chartScatter->axes())   { chartScatter->removeAxis(a);   a->deleteLater(); }

    pf::Config cfg;
    cfg.count          = spinCount->value();
    cfg.lambdaMin      = spinLMin->value();
    cfg.lambdaMax      = spinLMax->value();
    cfg.thresholdRatio = spinThreshold->value();
    cfg.K              = 3;
    cfg.maxIter        = 100;

    logMessage(QString("Генерация %1 значений...").arg(cfg.count));
    watcher->setFuture(QtConcurrent::run([cfg]() {
        return pf::processData(cfg);
    }));
}

// ---------------- Завершение ----------------
void MainWindow::onCalculationFinished()
{
    auto r = watcher->result();

    logMessage(QString("Сгенерировано %1 кривых, Y-порог = %2")
               .arg(r.lambdas.size()).arg(r.thresholdY, 0, 'f', 5));

    if (r.validIntervals.empty()) {
        logMessage("Все кривые ниже порога — уменьшите τ.");
        plotCurves(r, std::vector<int>(r.lambdas.size(), -1));
        // Пустое облако: только отсечённые
        pf::ClusterResult empty;
        plotScatter(r, empty);
        btnGenerate->setEnabled(true);
        return;
    }

    auto cl = pf::clusterIntervals(r.validIntervals, 3, 100);

    logMessage(QString("Кластеризация: %1 итераций, кластеров: %2")
               .arg(cl.iterations).arg(cl.summaries.size()));
    for (const auto& s : cl.summaries)
        logMessage(QString("  Кластер %1: n=%2 C=%3 R=%4 | %5")
                   .arg(s.id).arg(s.count)
                   .arg(s.meanCenter, 0, 'f', 2)
                   .arg(s.meanRelWidth, 0, 'f', 3)
                   .arg(QString::fromStdString(s.label)));

    // метки кластеров для всех кривых (-1 = отсечена)
    std::vector<int> clusterOf(r.lambdas.size(), -1);
    size_t j = 0;
    for (size_t i = 0; i < r.allIntervals.size() && j < cl.items.size(); ++i) {
        const auto& inv = r.allIntervals[i];
        if (inv.A >= 0 && (inv.B - inv.A) > 1) {
            clusterOf[i] = cl.items[j].cluster;
            ++j;
        }
    }

    // лог в файл
    QDir  dir(QCoreApplication::applicationDirPath());
    QFile f(dir.filePath("poisson_log.txt"));
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << "\n=== ЗАПУСК "
           << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
           << " ===\n";
        ts << "N=" << r.lambdas.size()
           << " Y=" << r.thresholdY
           << " кластеров=" << cl.summaries.size() << "\n";
        for (const auto& s : cl.summaries)
            ts << "  #" << s.id << " n=" << s.count
               << " | " << QString::fromStdString(s.label) << "\n";
    }

    plotCurves (r, clusterOf);
    plotScatter(r, cl);

    btnGenerate->setEnabled(true);
}

// ---------------- Вкладка 1: кривые + порог ----------------
void MainWindow::plotCurves(const pf::ProcessResult& r,
                            const std::vector<int>& clusters)
{
    if (r.lambdas.empty()) return;

    auto* ax = new QValueAxis(); ax->setTitleText("x");
    auto* ay = new QValueAxis(); ay->setTitleText("P(x)");
    chartCurves->addAxis(ax, Qt::AlignBottom);
    chartCurves->addAxis(ay, Qt::AlignLeft);

    const std::vector<QColor> palette = {
        QColor(60,120,255), QColor(40,180,90),
        QColor(255,140,0), QColor(180,80,200)
    };

    double maxY = 0, maxX = 0;
    int    black = 0;

    for (size_t i = 0; i < r.lambdas.size(); ++i) {
        const double lam = r.lambdas[i];
        const int    c   = (i < clusters.size()) ? clusters[i] : -1;

        QColor col = (c < 0) ? QColor(0,0,0) : palette[c % palette.size()];
        col.setAlpha(c < 0 ? 160 : 140);
        if (c < 0) ++black;

        auto* s = new QLineSeries();
        s->setPointsVisible(true);
        s->setPen(QPen(col, 1.0));

        int mx = static_cast<int>(lam + 5.0 * std::sqrt(lam) + 10);
        if (mx > maxX) maxX = mx;
        for (int x = 0; x <= mx; ++x) {
            const double p = pf::poissonPMF(x, lam);
            s->append(x, p);
            if (p > maxY) maxY = p;
        }
        chartCurves->addSeries(s);
        s->attachAxis(ax); s->attachAxis(ay);
    }

    const double xr = std::max(150.0, maxX);
    auto* tl = new QLineSeries();
    tl->append(0, r.thresholdY);
    tl->append(xr, r.thresholdY);
    tl->setPen(QPen(Qt::red, 2, Qt::DashLine));
    chartCurves->addSeries(tl);
    tl->attachAxis(ax); tl->attachAxis(ay);

    ax->setRange(0, xr);
    ay->setRange(0, maxY * 1.1);

    logMessage(QString("Кривые: серий=%1, чёрных (ниже Y=%2)=%3")
               .arg(chartCurves->series().size())
               .arg(r.thresholdY, 0, 'f', 5)
               .arg(black));
}
void MainWindow::plotScatter(const pf::ProcessResult& r,
                             const pf::ClusterResult& cl)
{
    // ---------- 0. Оси ----------
    auto* ax = new QValueAxis();
    ax->setTitleText("C — центр α-среза  (A+B)/2");
    ax->setLabelFormat("%.1f");

    auto* ay = new QValueAxis();
    ay->setTitleText("R — относительная ширина  W / max(C,1)");
    ay->setLabelFormat("%.3f");

    chartScatter->addAxis(ax, Qt::AlignBottom);
    chartScatter->addAxis(ay, Qt::AlignLeft);
    chartScatter->setBackgroundBrush(Qt::white);           // ← белый фон
    chartScatter->setPlotAreaBackgroundBrush(Qt::white);
    chartScatter->setPlotAreaBackgroundVisible(true);

    const std::vector<QColor> palette = {
        QColor(60,120,255), QColor(40,180,90),
        QColor(255,140,0),  QColor(180,80,200)
    };

    // ---------- 1. Границы по данным ----------
    double cMin =  std::numeric_limits<double>::infinity();
    double cMax = -std::numeric_limits<double>::infinity();
    double rMin =  std::numeric_limits<double>::infinity();
    double rMax = -std::numeric_limits<double>::infinity();
    int    totalValid = 0;
    int    cutCount   = 0;

    auto expandRange = [&](double C, double R) {
        cMin = std::min(cMin, C); cMax = std::max(cMax, C);
        rMin = std::min(rMin, R); rMax = std::max(rMax, R);
    };

    for (const auto& it : cl.items) { expandRange(it.C, it.R); ++totalValid; }
    for (const auto& inv : r.allIntervals) {
        if (inv.A < 0) continue;
        const int W = inv.B - inv.A;
        if (W > 1) continue;
        const double C = 0.5 * (inv.A + inv.B);
        const double R = W / std::max(C, 1.0);
        expandRange(C, R);
        ++cutCount;
    }

    qDebug() << "[scatter] items=" << cl.items.size()
             << "cut=" << cutCount
             << " C∈[" << cMin << ";" << cMax << "]"
             << " R∈[" << rMin << ";" << rMax << "]";

    // ---------- 2. Заглушка при отсутствии данных ----------
    if (!std::isfinite(cMin)) {
        ax->setRange(0, 10);
        ay->setRange(0, 1);
        chartScatter->setTitle("Облако кластеров: нет данных "
                               "(уменьшите τ или увеличьте N)");
        logMessage("Облако: нет точек для отображения.");
        return;
    }

    // ---------- 3. ЯВНО задаём диапазон осей ----------
    double padC = (cMax - cMin) * 0.10; if (padC < 1.0)   padC = 1.0;
    double padR = (rMax - rMin) * 0.10; if (padR < 0.05)  padR = 0.05;
    ax->setRange(cMin - padC, cMax + padC);
    ay->setRange(rMin - padR, rMax + padR);

    // ---------- 4. Точки по кластерам ----------
    std::map<int, QScatterSeries*> groups;
    for (const auto& it : cl.items) {
        const int c = it.cluster;
        if (c < 0) continue;
        auto bit = groups.find(c);
        if (bit == groups.end()) {
            auto* s = new QScatterSeries();
            s->setMarkerSize(14);                      // ← крупнее
            s->setMarkerShape(QScatterSeries::MarkerShapeCircle);
            const QString label = (cl.summaries.size() > (size_t)c)
                ? QString::fromStdString(cl.summaries[c].label)
                : QString("cluster %1").arg(c);
            s->setName(QString("Кластер %1 (n=%2)")
                        .arg(c).arg(0));               // заполним ниже
            s->setColor(palette[c % palette.size()]);
            s->setBorderColor(palette[c % palette.size()].darker(160));
            bit = groups.emplace(c, s).first;
        }
        bit->second->append(it.C, it.R);
    }
    // Проставим реальные размеры в имя серии
    for (auto& [c, s] : groups) {
        const int n = s->count();
        s->setName(QString("Кластер %1 (n=%2)").arg(c).arg(n));
        chartScatter->addSeries(s);
        s->attachAxis(ax);
        s->attachAxis(ay);
    }

    // ---------- 5. Центроиды (красный ромб) ----------
    if (!cl.items.empty()) {
        std::map<int, std::pair<double,double>> acc;
        std::map<int, int>                      cnt;
        for (const auto& it : cl.items) {
            acc[it.cluster].first  += it.C;
            acc[it.cluster].second += it.R;
            cnt[it.cluster]++;
        }
        auto* cent = new QScatterSeries();
        cent->setMarkerSize(22);
        cent->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
        cent->setColor(Qt::red);
        cent->setBorderColor(Qt::darkRed);
        cent->setName("Центроид");
        for (auto& [c, p] : acc)
            if (cnt[c] > 0)
                cent->append(p.first / cnt[c], p.second / cnt[c]);
        chartScatter->addSeries(cent);
        cent->attachAxis(ax);
        cent->attachAxis(ay);
    }

    // ---------- 6. Отсечённые (тёмно-серые) ----------
    if (cutCount > 0) {
        auto* cut = new QScatterSeries();
        cut->setMarkerSize(9);
        cut->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
        cut->setColor(QColor(90,90,90));
        cut->setBorderColor(QColor(40,40,40));
        cut->setName(QString("Отсечено (n=%1)").arg(cutCount));
        for (const auto& inv : r.allIntervals) {
            if (inv.A < 0) continue;
            const int W = inv.B - inv.A;
            if (W > 1) continue;
            const double C = 0.5 * (inv.A + inv.B);
            const double R = W / std::max(C, 1.0);
            cut->append(C, R);
        }
        chartScatter->addSeries(cut);
        cut->attachAxis(ax);
        cut->attachAxis(ay);
    }

    // ---------- 7. Заголовок ----------
    chartScatter->setTitle(
        QString("Облако кластеров (C, R): в кластерах=%1, отсечено=%2")
        .arg(cl.items.size()).arg(cutCount));

    logMessage(QString("Облако: точек в кластерах=%1, отсечено=%2, "
                       "C∈[%3;%4], R∈[%5;%6]")
               .arg(cl.items.size()).arg(cutCount)
               .arg(cMin, 0, 'f', 2).arg(cMax, 0, 'f', 2)
               .arg(rMin, 0, 'f', 3).arg(rMax, 0, 'f', 3));
}