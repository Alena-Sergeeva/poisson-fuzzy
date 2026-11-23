#pragma once
#include <QMainWindow>
#include <QFutureWatcher>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <vector>
#include <map>
#include <QString>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include <poisson_fuzzy/poisson_fuzzy.hpp>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onGenerateClicked();
    void onCalculationFinished();

private:
    void setupUI();
    void logMessage(const QString& msg);
    void plotCurves(const pf::ProcessResult& r, const std::vector<int>& clusters);
    void plotScatter(const pf::ProcessResult& r, const pf::ClusterResult& cl);

    QSpinBox*       spinCount     = nullptr;
    QDoubleSpinBox* spinLMin      = nullptr;
    QDoubleSpinBox* spinLMax      = nullptr;
    QDoubleSpinBox* spinThreshold = nullptr;
    QPushButton*    btnGenerate   = nullptr;
    QTextEdit*      logText       = nullptr;

    QTabWidget* tabs          = nullptr;
    QChart*     chartCurves   = nullptr;
    QChartView* viewCurves    = nullptr;
    QChart*     chartScatter  = nullptr;
    QChartView* viewScatter   = nullptr;

    QFutureWatcher<pf::ProcessResult>* watcher = nullptr;
};