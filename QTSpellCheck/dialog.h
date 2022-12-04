#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTextCursor>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog {
  Q_OBJECT

public:
  explicit Dialog(QWidget *parent = 0);
  ~Dialog();

protected slots:
	void highlightWords();
	void checkSpelling();

public slots:
	void OnDictionaryReplace(int index);

private:
  void replaceAll(int nPos, const QString &sOld, const QString &sNew);
  Ui::Dialog *ui;
  class SpellChecker *mSpellChecker;
  bool eventFilter( QObject * obj, QEvent * event );
  class QSignalMapper* mSignalMapper;
  QList<QAction*> mActionList;
  QTextCursor mSelectedWordCursor;
};

#endif // DIALOG_H
