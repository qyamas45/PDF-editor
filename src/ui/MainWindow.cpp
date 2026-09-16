#include "MainWindow.h"
#include "AnnotationToolBar.h"
#include "AnnotationOverlay.h"
#include "AnnotationPropertyBar.h"
#include "AnnotationRibbonBar.h"

 
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_document(new QPdfDocument(this))
    , m_pdfView(new QPdfView(this))
{
   
   setupUI();
   setupMenuBar();

}
 
MainWindow::~MainWindow() {
    // Ensure the file watcher is cleaned up properly
    if (m_fileWatcher) {
        m_fileWatcher->deleteLater();
        m_fileWatcher = nullptr;
    }
    trayIcon->hide(); 
}
void MainWindow::closeEvent(QCloseEvent *event) {
    // Hide the tray icon immediately so it doesn't leave a ghost icon on the taskbar
    trayIcon->hide(); 
    
    // Accept the close event normally, which shuts down the Qt event loop
    event->accept(); 
}
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    // Check for either a single click (Trigger) or a double click
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        
        // ONLY bring the window up. Never hide or close it from here.
        if (this->isMinimized()) {
            this->showNormal(); // Unminimize if it was minimized to the taskbar
        }
        
        this->show();           // Make sure it is visible
        this->raise();          // Bring it on top of other desktop apps
        this->activateWindow(); // Give it keyboard/mouse focus
    }
}

void MainWindow::forceQuitApplication() {
    this->close(); // Triggers the simplified closeEvent above
    qApp->quit();  // Hard kills the app loop in main.cpp
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);
    
    if (msg->message == 0x00A3 && msg->wParam == 3) {
        *result = 0;
        return true; // Swallows the double-click completely
    }

    // Catch when Windows tries to open or trigger the top-left system menu (shown in your image)
    if (msg->message == WM_SYSCOMMAND) {
        int command = msg->wParam & 0xFFF0;
        
        // SC_KEYMENU or double-click triggers on the system icon area
        if (command == SC_CLOSE) {
            // If it comes from a mouse action on the top-left icon (lParam is 0 or 3)
            if (msg->lParam == 0 || msg->lParam == 3) {
                *result = 0;
                return true; // Block the menu from opening and closing the app!
            }
        }
    }
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

/*
    Function name: buildDrawPanel
    Purpose: Configures the property bar with controls for the Draw tool.
    Details: This function adds a label indicating the "Draw" tool, followed by 
             controls for selecting stroke color and width. It includes color 
             swatches for quick selection and a spin box for custom stroke width. 
             The property bar's height is set to accommodate these controls.
*/
void MainWindow::setupUI()
{
    setWindowTitle("PDF Editor");
    resize(900, 700);
    trayMenu = new QMenu(this);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/tray_icon.png"));
    trayMenu->addAction("Quit", this, &MainWindow::forceQuitApplication);
    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    trayIcon->show();


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
        { ToolType::Erase,  "Erase",  AnnotationToolBar::makeEraseIcon()  },
        { ToolType::Shape,  "Shape",  AnnotationToolBar::makeShapeIcon()  }
    };
    // Set the tools on the toolbar and hide it until a PDF is loaded
    m_toolBar->setTools(tools);
    m_toolBar->hide();

    //Add propertyBar
    m_propBar = new AnnotationPropertyBar(m_pdfView->viewport());
    m_propBar->hide(); // Initially hidden until a PDF is loaded
    connect(m_toolBar, &AnnotationToolBar::toolSelected,
            this,  [this](ToolType type) {
                
        if (type == ToolType::None) {
            m_propBar->hide();
            return;
        }
        
        QPoint toolbarLocal(m_toolBar->buttonX(type)+50, m_toolBar->buttonY(type));
        QPoint viewportPos = m_pdfView->viewport()->mapFromGlobal(
            m_toolBar->mapToGlobal(toolbarLocal));
        int clampedY = qBound(0,
                              viewportPos.y(),
                              m_pdfView->viewport()->height() - m_propBar->height());
        m_propBar->move(viewportPos.x(), clampedY);
        m_propBar->raise();
        m_propBar->show();
        //qDebug() << "propBar visible:" << m_propBar->isVisible() << "geom:" << m_propBar->geometry();
        
    });
    // Rebuild property bar panel when tool changes
    connect(m_toolBar, &AnnotationToolBar::toolSelected,
            m_propBar, &AnnotationPropertyBar::setTool);

    // Right-side column: ribbon bar on top, PDF view below
    QWidget     *rightCol  = new QWidget(central);
    QVBoxLayout *rightVBox = new QVBoxLayout(rightCol);
    rightVBox->setContentsMargins(0, 0, 0, 0);
    rightVBox->setSpacing(0);

    //m_ribbonBar = new AnnotationRibbonBar(rightCol);
    //m_ribbonBar->hide(); // hidden until PDF is loaded

    //rightVBox->addWidget(m_ribbonBar);
    rightVBox->addWidget(m_pdfView, 1);

    // Add toolbar and right column to the main window layout
    hbox->addWidget(m_toolBar);
    hbox->addWidget(rightCol, 1);
    setCentralWidget(central);

    // Transparent overlay sits on top of the PDF viewport; receives the scroll
    // area so it can track scroll position and keep annotations page-anchored
    m_overlay = new AnnotationOverlay(m_pdfView);
    m_overlay->hide();

    connect(m_toolBar, &AnnotationToolBar::toolSelected,
            m_overlay,  &AnnotationOverlay::setActiveTool);

    // Any edit to the annotations makes the document dirty, which enables Save
    connect(m_overlay, &AnnotationOverlay::annotationsChanged,
            this,      [this]() { setSavedState(false); });

    // Forward property changes from the property bar to the overlay
    connect(m_propBar, &AnnotationPropertyBar::strokeColorChanged,
            m_overlay, &AnnotationOverlay::setStrokeColor);
    connect(m_propBar, &AnnotationPropertyBar::strokeWidthChanged,
            m_overlay, &AnnotationOverlay::setStrokeWidth);
    connect(m_propBar, &AnnotationPropertyBar::textColorChanged,
            m_overlay, &AnnotationOverlay::setTextColor);
    connect(m_propBar, &AnnotationPropertyBar::fontSizeChanged,
            m_overlay, &AnnotationOverlay::setFontSize);
    connect(m_propBar, &AnnotationPropertyBar::eraserRadiusChanged,
            m_overlay, &AnnotationOverlay::setEraserRadius);
}
/*
    Function name: setSavedState
    Purpose: Central switch for the "document has unsaved changes" state.
    Details: Keeps the isSaved flag, the enabled state of File > Save and the
             window title marker consistent. Save stays disabled while the
             document matches the file on disk (and when nothing is open).
*/
void MainWindow::setSavedState(bool saved)
{
    isSaved = saved;
    if (m_saveAction)
        m_saveAction->setEnabled(!saved && !m_currentPath.isEmpty());
    updateWindowTitle();
}

/*
    Function name: updateWindowTitle
    Purpose: Shows the open file name plus a "*" marker while changes are pending.
*/
void MainWindow::updateWindowTitle()
{
    if (m_currentPath.isEmpty()) {
        setWindowTitle("PDF Editor");
        return;
    }
    setWindowTitle("PDF Editor — " + QFileInfo(m_currentPath).fileName()
                   + (isSaved ? "" : " *"));
}

/*
    Function name: setupMenuBar
    Purpose: Sets up the main window's menu bar with file operations.
    Details: This function creates the "File" menu and adds actions for opening,
             saving, and quitting the application. It also sets up keyboard shortcuts
             for these actions.
*/
void MainWindow::setupMenuBar()
{
    QMenu   *fileMenu   = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open PDF...");
    openAction->setShortcut(QKeySequence::Open);  // Ctrl+O
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    QAction *saveAction = fileMenu->addAction("Save");
    saveAction->setShortcut(QKeySequence::SaveAs);  // Ctrl+S
    saveAction->setEnabled(false);
    connect(saveAction, &QAction::triggered, this, &MainWindow::overWritePdf);
    m_saveAction = saveAction;
    
    QAction *saveAsAction = fileMenu->addAction("&Save as PDF...");
    saveAsAction->setShortcut(QKeySequence::Save);  // Ctrl+Shift+S
    saveAsAction->setEnabled(false);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::savePdf);
    m_saveAsAction = saveAsAction;

    QAction *quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);  // Ctrl+Q
    
    // CHANGE THIS LINE: Route directly to forceQuitApplication
    connect(quitAction, &QAction::triggered, this, &MainWindow::forceQuitApplication);
}
/*
    Function name: openFile
    Purpose: Opens a PDF file and loads it into the application.
    Details: This function displays a file dialog for selecting a 
             PDF file. If a file is selected,
             it attempts to load the file into the PDF document. 
             If successful, it updates the
             window title and enables relevant UI elements. It 
             also sets up a file watcher to monitor the opened file for external changes, allowing the
*/
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

    // Ask about pending changes while the old document is still loaded —
    // overWritePdf() renders the current document into m_currentPath, so this
    // has to happen before either of them is replaced.
    if (!m_currentPath.isEmpty() && !isSaved) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes",
            "You have unsaved changes in the current document. Do you want to save them before opening a new file?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Cancel)
            return;
        if (reply == QMessageBox::Yes)
            overWritePdf();
    }

    if (m_document->load(path) != QPdfDocument::Error::None) {
        QMessageBox::warning(this, "Error", "Could not open: " + path);
        return;
    }
    //if it does exist, check the file size
    QFileInfo fi(path);
    qint64 fileSizeInBytes = fi.size();
    //if the PDF document is huge, then return
    //// 200 MB
    if (fileSizeInBytes > 200 * 1024 * 1024) { 
        QMessageBox::warning(this, "Error", "The selected PDF file is too large to open.");
        m_document->close();
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

    // Annotations belong to the previous document — drop them and start clean,
    // so Save stays disabled until the user actually edits this file.
    m_overlay->clearAnnotations();
    setSavedState(true);

    m_saveAsAction->setEnabled(true);
    m_toolBar->show();
    //m_ribbonBar->show();
    m_overlay->show();
    m_overlay->resize(m_pdfView->viewport()->size());
}
/*
    Function name: onSourceFileChanged
    Purpose: Handles the event when the source PDF file is modified or deleted 
             externally.
    Details: If the file is modified but still exists, 
             no action is taken. If the file is deleted or moved, a warning message 
             is shown to the user, and the file watcher stops monitoring the file 
             since it's no longer present. The user is advised to save their 
             current work to avoid losing annotations.
*/
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
/*
    Function name: savePdf
    Purpose: Exports the current PDF with annotations to a new file.
    Details: This function prompts the user to select a save location for the annotated PDF. 
             It then renders each page of the original PDF at a specified resolution, 
             applies the annotations from the overlay, and writes the result to a new PDF file. 
             Finally, it shows a confirmation message with the save location.

    Note: The export process involves rendering each page as an image, drawing annotations on top, 
          and then writing these images to a new PDF. This approach ensures that all annotations 
          are flattened into the output file, making it viewable in any standard PDF reader without 
          requiring special annotation support.
*/

void MainWindow::overWritePdf()
{
    const QString outPath = m_currentPath;
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

            p.setBrush(Qt::NoBrush);
            for (const Stroke &s : m_overlay->strokes()) {
                const QRectF bb = s.path.boundingRect();
                if (bb.bottom() < pageTopY[i] || bb.top() > pageTopY[i] + pageHPx)
                    continue;
                const qreal penW = qMax(1.0, s.width * exportScale / ss);
                p.setPen(QPen(s.color, penW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                p.drawPath(T.map(s.path));
            }

            for (const TextAnnotation &ann : m_overlay->textAnnotations()) {
                if (ann.position.y() < pageTopY[i] || ann.position.y() > pageTopY[i] + pageHPx)
                    continue;
                QFont font;
                font.setFamily("Arial");
                font.setPixelSize(qRound(ann.fontSize * exportScale / ss));
                p.setFont(font);
                p.setPen(ann.color);
                p.drawText(T.map(ann.position), ann.text);
            }
        }

        const QRect pageRect(0, 0,
                             pdfPainter.device()->width(),
                             pdfPainter.device()->height());
        pdfPainter.drawImage(pageRect, img);
    }
    pdfPainter.end();
    setSavedState(true);
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

            p.setBrush(Qt::NoBrush);
            for (const Stroke &s : m_overlay->strokes()) {
                const QRectF bb = s.path.boundingRect();
                if (bb.bottom() < pageTopY[i] || bb.top() > pageTopY[i] + pageHPx)
                    continue;
                const qreal penW = qMax(1.0, s.width * exportScale / ss);
                p.setPen(QPen(s.color, penW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                p.drawPath(T.map(s.path));
            }

            for (const TextAnnotation &ann : m_overlay->textAnnotations()) {
                if (ann.position.y() < pageTopY[i] || ann.position.y() > pageTopY[i] + pageHPx)
                    continue;
                QFont font;
                font.setFamily("Arial");
                font.setPixelSize(qRound(ann.fontSize * exportScale / ss));
                p.setFont(font);
                p.setPen(ann.color);
                p.drawText(T.map(ann.position), ann.text);
            }
        }

        const QRect pageRect(0, 0,
                             pdfPainter.device()->width(),
                             pdfPainter.device()->height());
        pdfPainter.drawImage(pageRect, img);
    }

    pdfPainter.end();

    // The annotations now live in outPath, but m_currentPath (the file Save
    // overwrites) is still untouched, so the document stays dirty.
    QMessageBox::information(this, "Saved",
        "Annotated PDF saved to:\n" + outPath);
}
