#include "MainWindow.h"
#include "AnnotationToolBar.h"
#include "AnnotationOverlay.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfView>
#include <QPdfWriter>
#include <QPageSize>
#include <QTransform>
#include <QWidget>

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

    m_pdfView->setDocument(m_document);
    m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);

    // Split layout: toolbar sidebar | PDF view
    QWidget *central = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(central);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    // Toolbar with annotation tools; initially hidden until a PDF is loaded
    m_toolBar = new AnnotationToolBar(central);
    // Define the available tools and their icons
    QVector<ToolItem> tools = {
        { ToolType::Select, "Select", AnnotationToolBar::makeSelectIcon() },
        { ToolType::Draw,   "Draw",   AnnotationToolBar::makeDrawIcon()   },
        { ToolType::Text,   "Text",   AnnotationToolBar::makeTextIcon()   },
    };
    // Set the tools on the toolbar and hide it until a PDF is loaded
    m_toolBar->setTools(tools);
    m_toolBar->hide();

    // Add toolbar and PDF view to the main window layout
    hbox->addWidget(m_toolBar);
    hbox->addWidget(m_pdfView, 1);
    setCentralWidget(central);

    // Transparent overlay sits on top of the PDF viewport; receives the scroll
    // area so it can track scroll position and keep annotations page-anchored
    m_overlay = new AnnotationOverlay(m_pdfView);
    m_overlay->hide();

    connect(m_toolBar, &AnnotationToolBar::toolSelected,
            m_overlay,  &AnnotationOverlay::setActiveTool);
}

void MainWindow::setupMenuBar()
{
    QMenu   *fileMenu   = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open PDF...");
    openAction->setShortcut(QKeySequence::Open);  // Ctrl+O
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    QAction *saveAction = fileMenu->addAction("&Save as PDF...");
    saveAction->setShortcut(QKeySequence::Save);  // Ctrl+S
    saveAction->setEnabled(false);
    connect(saveAction, &QAction::triggered, this, &MainWindow::savePdf);
    m_saveAction = saveAction;

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

    // Stop watching the previous file if any
    if (!m_currentPath.isEmpty() && m_fileWatcher)
        m_fileWatcher->removePath(m_currentPath);

    m_currentPath = path;
    // Watch the opened file for external changes to auto-reload
    if (!m_fileWatcher) {
        // Note: QFileSystemWatcher can have issues on 
        // some platforms if the file is replaced
        // (e.g. by some PDF editors that save 
        // by writing to a temp file and
        m_fileWatcher = new QFileSystemWatcher(this);
        connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                this,          &MainWindow::onSourceFileChanged);
    }
    m_fileWatcher->addPath(m_currentPath);

    setWindowTitle("PDF Editor — " + QFileInfo(path).fileName());

    m_saveAction->setEnabled(true);
    m_toolBar->show();
    m_overlay->show();
    m_overlay->resize(m_pdfView->viewport()->size());
}
void MainWindow::onSourceFileChanged(const QString &path)
{
    if (QFileInfo::exists(path))
        return; // modified but still present — no action needed

    QMessageBox::warning(
        this,
        "Source File Deleted",
        "The original file \"" + QFileInfo(path).fileName() + "\" has been deleted or moved.\n\n"
        "Your current work is still open. Use File > Save as PDF to keep your annotations."
    );

    // Stop watching — file is gone, further signals would be noise
    m_fileWatcher->removePath(path);
}
void MainWindow::savePdf()
{
    if (m_document->pageCount() == 0) return;

    const QString outPath = QFileDialog::getSaveFileName(
        this, "Save Annotated PDF", {}, "PDF Files (*.pdf)");
    if (outPath.isEmpty()) return;

    const qreal exportDpi   = 150.0;
    const qreal exportScale = exportDpi / 72.0;

    const QMargins margins = m_pdfView->documentMargins();
    const int      spacing = m_pdfView->pageSpacing();
    const qreal viewWidth  = m_pdfView->viewport()->width()
                             - margins.left() - margins.right();
    const int pageCount    = m_document->pageCount();

    QVector<qreal> screenScaleV(pageCount), pageTopY(pageCount);
    qreal docY = margins.top();
    for (int i{}; i < pageCount; ++i) {
        const QSizeF sz  = m_document->pagePointSize(i);
        screenScaleV[i]  = viewWidth / sz.width();
        pageTopY[i]      = docY;
        docY            += sz.height() * screenScaleV[i] + spacing;
    }

    QPdfWriter writer(outPath);
    writer.setCreator("PDF Editor");
    QPainter pdfPainter;

    for (int i{}; i < pageCount; ++i) {
        const QSizeF pageSizePt = m_document->pagePointSize(i);
        const qreal  ss         = screenScaleV[i];
        const qreal  pageHPx    = pageSizePt.height() * ss;

        const QSize exportSizePx(qRound(pageSizePt.width()  * exportScale),
                                 qRound(pageSizePt.height() * exportScale));

        writer.setPageSize(QPageSize(pageSizePt, QPageSize::Point));
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));

        if (i == 0) pdfPainter.begin(&writer);
        else        writer.newPage();

        QImage img = m_document->render(i, exportSizePx);

        {
            QPainter p(&img);
            p.setRenderHint(QPainter::Antialiasing);

            QTransform T;
            T.translate(-margins.left(), -pageTopY[i]);
            T.scale(exportScale / ss, exportScale / ss);

            const qreal penW = qMax(1.0, 2.5 * exportScale / ss);
            p.setPen(QPen(QColor(220, 50, 50, 200), penW,
                          Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            for (const QPainterPath &stroke : m_overlay->strokes()) {
                const QRectF bb = stroke.boundingRect();
                if (bb.bottom() < pageTopY[i] || bb.top() > pageTopY[i] + pageHPx)
                    continue;
                p.drawPath(T.map(stroke));
            }

            QFont font;
            font.setFamily("Arial");
            font.setPixelSize(qRound(16.0 * exportScale / ss));
            p.setFont(font);
            p.setPen(QColor(30, 30, 180, 220));
            for (const TextAnnotation &ann : m_overlay->textAnnotations()) {
                if (ann.position.y() < pageTopY[i] || ann.position.y() > pageTopY[i] + pageHPx)
                    continue;
                p.drawText(T.map(ann.position), ann.text);
            }
        }

        const QRect pageRect(0, 0,
                             pdfPainter.device()->width(),
                             pdfPainter.device()->height());
        pdfPainter.drawImage(pageRect, img);
    }

    pdfPainter.end();
    QMessageBox::information(this, "Saved",
        "Annotated PDF saved to:\n" + outPath);
}
