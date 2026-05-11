#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPdfDocument;
class QPdfView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void openFile();

private:
    QPdfDocument *m_document;
    QPdfView     *m_pdfView;

    void setupUI();
    void setupMenuBar();
};

#endif // MAINWINDOW_H
