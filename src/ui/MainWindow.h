#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPdfDocument;
class QPdfView;
class AnnotationToolBar;
class AnnotationOverlay;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void openFile();
    void savePdf();

private:
    QPdfDocument      *m_document;
    QPdfView          *m_pdfView;
    AnnotationToolBar *m_toolBar    = nullptr;
    AnnotationOverlay *m_overlay    = nullptr;
    QAction           *m_saveAction = nullptr;

    void setupUI();
    void setupMenuBar();
};

#endif // MAINWINDOW_H
