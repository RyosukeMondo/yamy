#include "dialog_shortcuts_qt.h"
#include <QHeaderView>
#include <QSettings>
#include <QLabel>

DialogShortcutsQt::DialogShortcutsQt(QWidget* parent)
    : QDialog(parent)
    , m_searchBox(nullptr)
    , m_shortcutTable(nullptr)
    , m_btnClose(nullptr)
{
    setWindowTitle("Keyboard Shortcuts");
    setMinimumSize(500, 400);
    resize(600, 500);

    setupUI();
    loadShortcuts();
}

DialogShortcutsQt::~DialogShortcutsQt()
{
}

void DialogShortcutsQt::onSearchTextChanged(const QString& text)
{
    QString searchTerm = text.toLower();

    for (int row = 0; row < m_shortcutTable->rowCount(); ++row) {
        bool matches = false;

        // Search in action and shortcut columns
        for (int col = 0; col < m_shortcutTable->columnCount(); ++col) {
            QTableWidgetItem* item = m_shortcutTable->item(row, col);
            if (item && item->text().toLower().contains(searchTerm)) {
                matches = true;
                break;
            }
        }

        m_shortcutTable->setRowHidden(row, !matches);
    }
}

void DialogShortcutsQt::onClose()
{
    close();
}

void DialogShortcutsQt::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search box
    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLabel* searchLabel = new QLabel("Search:");
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Type to filter shortcuts...");
    m_searchBox->setClearButtonEnabled(true);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &DialogShortcutsQt::onSearchTextChanged);
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(m_searchBox);
    mainLayout->addLayout(searchLayout);

    // Shortcuts table
    m_shortcutTable = new QTableWidget();
    m_shortcutTable->setColumnCount(3);
    m_shortcutTable->setHorizontalHeaderLabels({"Action", "Shortcut", "Category"});
    m_shortcutTable->horizontalHeader()->setStretchLastSection(true);
    m_shortcutTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_shortcutTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_shortcutTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_shortcutTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_shortcutTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_shortcutTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_shortcutTable->setSortingEnabled(true);
    m_shortcutTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_shortcutTable);

    // Close button
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnClose = new QPushButton("Close");
    m_btnClose->setDefault(true);
    connect(m_btnClose, &QPushButton::clicked, this, &DialogShortcutsQt::onClose);
    btnLayout->addWidget(m_btnClose);

    mainLayout->addLayout(btnLayout);
}

void DialogShortcutsQt::loadShortcuts()
{
    // Load configured shortcuts from settings
    QSettings settings("YAMY", "YAMY");

    // Quick Config Switch hotkey
    QString quickSwitchSeq = settings.value("hotkeys/quickSwitch/sequence", "Ctrl+Alt+C").toString();
    if (!quickSwitchSeq.isEmpty()) {
        addShortcut("Quick Config Switch", quickSwitchSeq, "Global");
    }

    // Reload hotkey (if configured)
    QString reloadSeq = settings.value("hotkeys/reload/sequence", "").toString();
    if (!reloadSeq.isEmpty()) {
        addShortcut("Reload Configuration", reloadSeq, "Global");
    }

    // Investigate hotkey (if configured)
    QString investigateSeq = settings.value("hotkeys/investigate/sequence", "").toString();
    if (!investigateSeq.isEmpty()) {
        addShortcut("Open Investigate Dialog", investigateSeq, "Global");
    }

    // Log hotkey (if configured)
    QString logSeq = settings.value("hotkeys/log/sequence", "").toString();
    if (!logSeq.isEmpty()) {
        addShortcut("Open Log Dialog", logSeq, "Global");
    }

    // Application shortcuts (built-in)
    addShortcut("Toggle Enable/Disable", "Double-click tray", "Application");
    addShortcut("Reload Configuration", "Middle-click tray", "Application");

    // Dialog shortcuts
    addShortcut("Close Dialog", "Esc", "Dialog");
    addShortcut("Find in Log", "Ctrl+F", "Dialog");
    addShortcut("Clear Search", "Esc (in search)", "Dialog");
    addShortcut("Navigate Results", "↑/↓", "Dialog");
    addShortcut("Copy Selection", "Ctrl+C", "Dialog");
    addShortcut("Select All", "Ctrl+A", "Dialog");

    // Sort by category initially
    m_shortcutTable->sortByColumn(2, Qt::AscendingOrder);
}

void DialogShortcutsQt::addShortcut(const QString& action, const QString& shortcut,
                                     const QString& category)
{
    int row = m_shortcutTable->rowCount();
    m_shortcutTable->insertRow(row);

    QTableWidgetItem* actionItem = new QTableWidgetItem(action);
    QTableWidgetItem* shortcutItem = new QTableWidgetItem(shortcut);
    QTableWidgetItem* categoryItem = new QTableWidgetItem(category);

    // Make shortcut column bold
    QFont boldFont = shortcutItem->font();
    boldFont.setBold(true);
    shortcutItem->setFont(boldFont);

    m_shortcutTable->setItem(row, 0, actionItem);
    m_shortcutTable->setItem(row, 1, shortcutItem);
    m_shortcutTable->setItem(row, 2, categoryItem);
}
