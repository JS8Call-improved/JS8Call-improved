#include "TransmitTextSourceMirror.h"

#include <QTextCursor>
#include <QTextDocument>

TransmitTextEdit::source_mirror::source_mirror(TransmitTextEdit *owner)
    : owner_(owner), document_(new QTextDocument(owner)) {
    document_->setUndoRedoEnabled(true);
}

QString TransmitTextEdit::source_mirror::plainText() const {
    return document_->toPlainText();
}

TransmitTextEdit::source_mirror::CursorState
TransmitTextEdit::source_mirror::captureCursorState() const {
    const QTextCursor cursor = owner_->textCursor();
    return {cursor.anchor(), cursor.position()};
}

void TransmitTextEdit::source_mirror::restoreCursorState(
    const CursorState &state) const {
    QTextCursor cursor = owner_->textCursor();
    const int length = owner_->QTextEdit::toPlainText().size();
    const int anchor = qBound(0, state.anchor, length);
    const int position = qBound(0, state.position, length);
    cursor.setPosition(anchor);
    cursor.setPosition(position, QTextCursor::KeepAnchor);
    owner_->setTextCursor(cursor);
}

void TransmitTextEdit::source_mirror::replaceVisiblePlainText(
    const QString &text, bool keepCursor) const {
    auto rel =
        TransmitTextEdit::relativeTextCursorPosition(owner_->textCursor());
    auto c = owner_->textCursor();
    c.beginEditBlock();
    c.movePosition(QTextCursor::Start);
    c.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    c.removeSelectedText();
    c.insertText(text);
    c.endEditBlock();

    if (keepCursor) {
        c.movePosition(QTextCursor::End);
        c.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor,
                       rel.first);
        c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                       rel.second - rel.first);
        owner_->setTextCursor(c);
    }
}

void TransmitTextEdit::source_mirror::syncDocument(const QString &text) {
    if (document_->toPlainText() == text) {
        return;
    }

    QTextCursor cursor(document_);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    cursor.insertText(text);
    cursor.endEditBlock();
}

void TransmitTextEdit::source_mirror::replaceUnsentText(int sent,
                                                        const QString &text) {
    QTextCursor cursor(document_);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                        sent);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(text);
    cursor.endEditBlock();
}

void TransmitTextEdit::source_mirror::applyChange(int pos, int rem,
                                                  const QString &insertedText) {
    QTextCursor cursor(document_);
    const int textLength = document_->toPlainText().size();
    cursor.setPosition(qBound(0, pos, textLength));
    cursor.beginEditBlock();
    if (rem > 0) {
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor, rem);
        cursor.removeSelectedText();
    }
    if (!insertedText.isEmpty()) {
        cursor.insertText(insertedText);
    }
    cursor.endEditBlock();
}

void TransmitTextEdit::source_mirror::syncDisplayFromSource(
    const CursorState &state, bool keepCursor) const {
    const QString sourceText = plainText();
    if (owner_->QTextEdit::toPlainText() != sourceText) {
        owner_->QTextEdit::setPlainText(sourceText);
    }

    if (keepCursor) {
        restoreCursorState(state);
    } else {
        QTextCursor cursor = owner_->textCursor();
        cursor.movePosition(QTextCursor::End);
        owner_->setTextCursor(cursor);
    }
}

void TransmitTextEdit::source_mirror::updateSentCache() const {
    const QString text = plainText();
    owner_->m_sent = qBound(0, owner_->m_sent, text.size());
    owner_->m_textSent = text.left(owner_->m_sent);
}

QTextDocument *TransmitTextEdit::source_mirror::document() const {
    return document_;
}

void TransmitTextEdit::source_mirror::beginSuppression() {
    ++suppressionDepth_;
}

void TransmitTextEdit::source_mirror::endSuppression() {
    if (suppressionDepth_ == 0) {
        return;
    }

    --suppressionDepth_;
}

bool TransmitTextEdit::source_mirror::isSuppressing() const {
    return suppressionDepth_ > 0;
}

void TransmitTextEdit::source_mirror::beginSyncFromSource() { ++syncDepth_; }

void TransmitTextEdit::source_mirror::endSyncFromSource() {
    if (syncDepth_ == 0) {
        return;
    }

    --syncDepth_;
}

bool TransmitTextEdit::source_mirror::isSyncingFromSource() const {
    return syncDepth_ > 0;
}

void TransmitTextEdit::source_mirror::beginInternalMutation() {
    ++internalMutationDepth_;
}

void TransmitTextEdit::source_mirror::endInternalMutation() {
    if (internalMutationDepth_ == 0) {
        return;
    }

    --internalMutationDepth_;
}

bool TransmitTextEdit::source_mirror::isInternalMutationActive() const {
    return internalMutationDepth_ > 0;
}
