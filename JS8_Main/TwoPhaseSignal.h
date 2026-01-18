#ifndef TWOPHASESIGNAL_H
#define TWOPHASESIGNAL_H

#include <QObject>

class TwoPhaseSignal : public QObject {
    Q_OBJECT
  public:
    TwoPhaseSignal();
    ~TwoPhaseSignal();

  public slots:
    /**
     * Called when the plumbing has been completed.
     * The object should react by fireing its signals.
     */
    virtual void onPlumbingCompleted() const = 0;
};

#endif
