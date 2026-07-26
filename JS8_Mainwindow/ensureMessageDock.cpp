/** \file
 * @brief UI_Constructor::ensureMessageDock()
 * member function of the UI_Constructor class
 *
 * Lazily constructs the dockable Message Inbox panel (messageDock_ /
 * messagePanel_) on first use, including its custom toolbar (callsign
 * filter box, clear/float/close buttons) and the signal connections that
 * keep the "Show Message Inbox" menu action, dock visibility, and floating
 * window behavior in sync.
 *
 * @note Must be called before restoreState() in readSettings() so the
 * toolbar exists before saved dock visibility is restored.
 */

#include "JS8_UI/mainwindow.h"

void UI_Constructor::ensureMessageDock()
{
    if (messageDock_) return;

    messagePanel_ = new MessagePanel(inboxPath(), this);

    messageDock_ = new QDockWidget(tr("Message Inbox"), this);
    messageDock_->setObjectName("messageInboxDock"); // important for save/restoreState
    messageDock_->setWidget(messagePanel_);

    // Choose where it can dock:
    messageDock_->setAllowedAreas(Qt::LeftDockWidgetArea |
                                  Qt::RightDockWidgetArea |
                                  Qt::BottomDockWidgetArea);

    // Choose behavior:
    messageDock_->setFeatures(QDockWidget::DockWidgetMovable |
                              QDockWidget::DockWidgetFloatable |
                              QDockWidget::DockWidgetClosable);

    // Initial placement:
    addDockWidget(Qt::RightDockWidgetArea, messageDock_);

    // Optional: closing hides (default); ensure no auto-delete:
    messageDock_->setAttribute(Qt::WA_DeleteOnClose, false);

    // --- Build the custom toolbar ---
    if (!m_inboxFilterEdit_) {
        auto *titleBar = new QWidget(messageDock_);
        auto *h = new QHBoxLayout(titleBar);
        h->setContentsMargins(6, 2, 6, 2);
        auto *lbl = new QLabel("Inbox:", titleBar);
        auto *edit = new QLineEdit(titleBar);
        edit->setPlaceholderText("Filter by callsign");
        // Restrict width of the QLineEdit entry widget so the rest of the tool
        // bar remains draggable
        {
            const int chars = 10;
            const QFontMetrics fm(edit->font());
            const int textWidth = fm.horizontalAdvance(QString(chars, QChar('W')));
            edit->setMaximumWidth(textWidth + 10);
            edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        auto *floatBtn = new QToolButton(titleBar);
        floatBtn->setAutoRaise(true);
        floatBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
        floatBtn->setToolTip("Toggle floating");
        auto *closeBtn = new QToolButton(titleBar);
        closeBtn->setAutoRaise(true);
        closeBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
        closeBtn->setToolTip("Close");
        auto *clearBtn = new QToolButton(titleBar);
        clearBtn->setText("<-clear");
        clearBtn->setToolTip("Clear filter");
        clearBtn->setAutoRaise(true);

        h->addWidget(closeBtn);
        h->addWidget(floatBtn);
        h->addStretch();
        h->addWidget(lbl);
        h->addWidget(edit);
        h->addWidget(clearBtn);

        titleBar->setLayout(h);
        messageDock_->setTitleBarWidget(titleBar);

        m_inboxFilterEdit_ = edit;

        connect(edit, &QLineEdit::textChanged, this, [this](const QString &txt) {
            if (messagePanel_) messagePanel_->setCall(txt.trimmed().isEmpty() ? "%" : txt.trimmed().toUpper());
        });
        connect(clearBtn, &QToolButton::clicked, this, [this]() {
            if (m_inboxFilterEdit_) m_inboxFilterEdit_->clear();
        });
        connect(closeBtn, &QToolButton::clicked, messageDock_, &QDockWidget::hide);
        connect(floatBtn, &QToolButton::clicked, this, [this]() {
            if (messageDock_) messageDock_->setFloating(!messageDock_->isFloating());
        });
    }

    // Make the menu action reflect visibility automatically:
    ui->actionShow_Message_Inbox->setCheckable(true);
    ui->actionShow_Message_Inbox->setChecked(messageDock_->isVisible());
    connect(messageDock_, &QDockWidget::visibilityChanged, this,
            [this](bool visible) {
                QSignalBlocker b(ui->actionShow_Message_Inbox);
                ui->actionShow_Message_Inbox->setChecked(visible);
            });
    
    // When floating the message inbox becomes a Qt::Window rather than a Qt::Tool
    // This gives it normal window controls that allow minimizing, changing focus, etc..
    connect(messageDock_, &QDockWidget::topLevelChanged, this, [this](bool floating) {
        if (floating) {
            messageDock_->setWindowFlags(Qt::Window
                                          | Qt::WindowCloseButtonHint
                                          | Qt::WindowMinMaxButtonsHint);
            messageDock_->show();
        }
    });

    // Handle reply function
    connect(messagePanel_, &MessagePanel::replyMessage, this,
                [this](const QString &text) {
                    addMessageText(text, true, true);
                    refreshInboxCounts();
                    displayCallActivity();
                });

    connect(messagePanel_, &MessagePanel::countsUpdated, this, [this]() {
            refreshInboxCounts();
            displayCallActivity();
        });
}
