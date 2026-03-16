#ifndef TRANSMITTEXTSOURCEMIRROR_H
#define TRANSMITTEXTSOURCEMIRROR_H

#include "TransmitTextEdit.h"

class QTextDocument;

class TransmitTextEdit::source_mirror final {
  public:
    struct CursorState {
        int anchor = 0;
        int position = 0;
    };

    explicit source_mirror(TransmitTextEdit *owner);

    QString plainText() const;
    CursorState captureCursorState() const;
    void restoreCursorState(const CursorState &state) const;
    void replaceVisiblePlainText(const QString &text, bool keepCursor) const;
    void syncDocument(const QString &text);
    void replaceUnsentText(int sent, const QString &text);
    void applyChange(int pos, int rem, const QString &insertedText);
    void syncDisplayFromSource(const CursorState &state,
                               bool keepCursor) const;
    void updateSentCache() const;

    QTextDocument *document() const;

    void beginSuppression();
    void endSuppression();
    bool isSuppressing() const;

    void beginSyncFromSource();
    void endSyncFromSource();
    bool isSyncingFromSource() const;

    void beginInternalMutation();
    void endInternalMutation();
    bool isInternalMutationActive() const;

  private:
    TransmitTextEdit *owner_;
    QTextDocument *document_ = nullptr;
    int suppressionDepth_ = 0;
    int syncDepth_ = 0;
    int internalMutationDepth_ = 0;
};

#endif // TRANSMITTEXTSOURCEMIRROR_H
