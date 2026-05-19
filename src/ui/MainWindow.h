#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFileSystemWatcher>
class QPdfDocument;
class QPdfView;
class QFileSystemWatcher;
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
    void onSourceFileChanged(const QString &path);

private:
    QPdfDocument       *m_document;
    QPdfView           *m_pdfView;
    AnnotationToolBar  *m_toolBar     = nullptr;
    AnnotationOverlay  *m_overlay     = nullptr;
    QAction            *m_saveAction  = nullptr;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QString             m_currentPath;

    void setupUI();
    void setupMenuBar();
};

#endif // MAINWINDOW_H
