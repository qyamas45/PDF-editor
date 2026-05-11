#include "MainWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QPdfDocument>
#include <QPdfView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_document(new QPdfDocument(this))
    , m_pdfView(new QPdfView(this))
{
    setupUI();
    setupMenuBar();
}

void MainWindow::setupUI()
{
    setWindowTitle("PDF Editor");
    resize(900, 700);

    // QPdfView is the scrollable PDF renderer — set it as the central widget
    m_pdfView->setDocument(m_document);
    m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    setCentralWidget(m_pdfView);
}

void MainWindow::setupMenuBar()
{
    QMenu   *fileMenu   = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open PDF...");
    openAction->setShortcut(QKeySequence::Open);  // Ctrl+O
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    QAction *quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);  // Ctrl+Q
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Open PDF",
        {},
        "PDF Files (*.pdf)"
    );

    if (path.isEmpty())
        return;

    if (m_document->load(path) != QPdfDocument::Error::None) {
        QMessageBox::warning(this, "Error", "Could not open: " + path);
        return;
    }

    setWindowTitle("PDF Editor — " + QFileInfo(path).fileName());
}
