#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QDateTime>

class Engine;

class StatusWidgetQt : public QWidget
{
    Q_OBJECT

public:
    explicit StatusWidgetQt(Engine* engine, QWidget* parent = nullptr);
    virtual ~StatusWidgetQt();

private slots:
    void updateStats();

private:
    void setupUI();

    Engine* m_engine;
    QTimer* m_updateTimer;
    QDateTime m_startTime;

    QLabel* m_labelUptime;
    QLabel* m_labelKeysProcessed;
    QLabel* m_labelActiveWindow;
};
