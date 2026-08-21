#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFileSystemWatcher>
#include <QSystemTrayIcon>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfView>
#include <QPdfWriter>
#include <QPageSize>
#include <QTransform>
#include <QWidget>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

class QPdfDocument;
class QPdfView;
class QFileSystemWatcher;
class AnnotationToolBar;
class AnnotationOverlay;
class AnnotationPropertyBar;
class AnnotationRibbonBar;
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void closeEvent(QCloseEvent *event) override;
    void forceQuitApplication();
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
private slots:
    void openFile();
    void savePdf();
    void overWritePdf();
    void onSourceFileChanged(const QString &path);

private:
    // True while the on-screen document matches what is on disk. A freshly
    // opened (or just-saved) file starts clean, so Save is disabled.
    bool                    isSaved = true;
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QPdfDocument            *m_document;
    QPdfView                *m_pdfView;
    AnnotationToolBar       *m_toolBar     = nullptr;
    AnnotationPropertyBar   *m_propBar     = nullptr;
    AnnotationOverlay       *m_overlay     = nullptr;
    AnnotationRibbonBar     *m_ribbonBar   = nullptr;
    QAction                 *m_saveAction  = nullptr;
    QAction                 *m_saveAsAction = nullptr;
    QFileSystemWatcher      *m_fileWatcher = nullptr;
    QString                  m_currentPath;

    void setupUI();
    void setupMenuBar();

    // Single place that keeps isSaved, the Save action and the title marker in sync.
    void setSavedState(bool saved);
    void updateWindowTitle();
 
};

#endif // MAINWINDOW_H
