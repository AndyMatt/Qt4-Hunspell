#include "./dialog.h"

#include <QDir>
#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QSignalMapper>

#include "ui_dialog.h"
#include "./spellchecker.h"
#include "./spellcheckdialog.h"

Dialog::Dialog(QWidget *parent)
: QDialog(parent),
ui(new Ui::Dialog),
mSpellChecker(NULL)
{
	ui->setupUi(this);
	setMouseTracking(true);
	QObject::connect(ui->buttonCheckSpelling, SIGNAL(clicked()), this, SLOT(checkSpelling()));
	QObject::connect(ui->textEdit, SIGNAL(textChanged()), this, SLOT(highlightWords()));

	mSignalMapper = new QSignalMapper (this) ;
	connect(mSignalMapper, SIGNAL(mapped(int)), this, SLOT(OnDictionaryReplace(int)));

	ui->textEdit->installEventFilter( this );

	QString language = QString("en_US");

	QString dictPath = qApp->applicationDirPath() + "/dicts/";

	if (!dictPath.endsWith('/')) 
	{
		dictPath.append('/');
	}

	dictPath += language;

	// Save user dictionary in same folder as application
	QString userDict = "userDict_" + language + ".txt";

	mSpellChecker = new SpellChecker(dictPath, userDict);
}

Dialog::~Dialog() {
  delete ui;
}

bool Dialog::eventFilter( QObject * obj, QEvent * event )
{
	if ( ( obj == ui->textEdit ) 
		&& ( event->type() == QEvent::MouseButtonPress ) )
	{
		QMouseEvent* mouseEvent = reinterpret_cast< QMouseEvent * >( event );

		if(mouseEvent->buttons() & 0x00000002 /*MouseButton::RightButton*/ )
		{
			for(int i = 0; i < mActionList.size(); i++)
			{
				mSignalMapper->removeMappings(mActionList[i]);
				delete mActionList[i];
			}
			mActionList.clear();

			QMenu* menu = ui->textEdit->createStandardContextMenu();

			mSelectedWordCursor = ui->textEdit->cursorForPosition(mouseEvent->pos());
			mSelectedWordCursor.select(QTextCursor::WordUnderCursor);
			QString strWord = mSelectedWordCursor.selectedText();

			if(!mSpellChecker->spell(strWord))
			{
				QStringList suggestions = mSpellChecker->suggest(strWord);
				QAction* itr = menu->actions().first();
				if(suggestions.count() == 0)
				{
					QAction* newAction = new QAction("No Suggestions",this);
					newAction->setDisabled(true);
					menu->insertAction(itr,newAction);
				}
				else
				{
					for(int i = 0; i < suggestions.count() && i < 5; i++)
					{
						mActionList.push_back(new QAction(suggestions[i],this));
						connect(mActionList[i], SIGNAL(triggered()), mSignalMapper, SLOT(map()) );
						mSignalMapper->setMapping(mActionList[i], i);
					}
				}

				menu->insertActions(itr,mActionList);
				menu->insertSeparator(itr);
			}			

			menu->exec( mouseEvent->globalPos() );
			delete menu;
			return true;
		}
	}
	return false;
}

void Dialog::OnDictionaryReplace(int index)
{
	mSelectedWordCursor.insertText(mActionList[index]->text());
}

void Dialog::highlightWords()
{
	QObject::disconnect(ui->textEdit, SIGNAL(textChanged()), this, SLOT(highlightWords()));

	QTextCharFormat highlightFormat;
	highlightFormat.setUnderlineColor(QColor("red"));
	highlightFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

	ui->textEdit->document()->setPlainText(ui->textEdit->document()->toPlainText());

	QTextCursor cursor(ui->textEdit->document());

	while (!cursor.atEnd()) 
	{
		QCoreApplication::processEvents();
		cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
		QString word = cursor.selectedText();

		while (!word.isEmpty() && !word.at(0).isLetter() && cursor.anchor() < cursor.position()) 
		{
			int cursorPos = cursor.position();
			cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
			cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
			word = cursor.selectedText();
		}

		if (!word.isEmpty() && !mSpellChecker->spell(word)) 
		{
			cursor.setCharFormat(highlightFormat);
			QCoreApplication::processEvents();
		}
		
		cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
	}
	QObject::connect(ui->textEdit, SIGNAL(textChanged()), this, SLOT(highlightWords()));
}

void Dialog::checkSpelling() {
	SpellCheckDialog *checkDialog = new SpellCheckDialog(mSpellChecker, this);

	QTextCharFormat highlightFormat;
	highlightFormat.setBackground(QBrush(QColor(0xff, 0x60, 0x60)));
	highlightFormat.setForeground(QBrush(QColor(0, 0, 0)));

	QTextCursor oldCursor = ui->textEdit->textCursor();
	QTextCursor cursor(ui->textEdit->document());
	QList<QTextEdit::ExtraSelection> esList;
	while (!cursor.atEnd()) 
	{
		QCoreApplication::processEvents();
		cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
		QString word = cursor.selectedText();

		while (!word.isEmpty() && !word.at(0).isLetter() && cursor.anchor() < cursor.position()) 
		{
			int cursorPos = cursor.position();
			cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
			cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
			word = cursor.selectedText();
		}

		if (!word.isEmpty() && !mSpellChecker->spell(word)) 
		{
			QTextCursor tmpCursor(cursor);
			tmpCursor.setPosition(cursor.anchor());
			ui->textEdit->setTextCursor(tmpCursor);
			ui->textEdit->ensureCursorVisible();

			QTextEdit::ExtraSelection es;
			es.cursor = cursor;
			es.format = highlightFormat;

			esList << es;
			ui->textEdit->setExtraSelections(esList);
			QCoreApplication::processEvents();

			SpellCheckDialog::SpellCheckAction spellResult = checkDialog->checkWord(word);

			esList.clear();
			ui->textEdit->setExtraSelections(esList);
			QCoreApplication::processEvents();

			if (spellResult == SpellCheckDialog::AbortCheck)
				break;

			switch (spellResult) 
			{
				case SpellCheckDialog::ReplaceOnce:
					cursor.insertText(checkDialog->replacement());
				break;
				case SpellCheckDialog::ReplaceAll:
					replaceAll(cursor.position(), word, checkDialog->replacement());
				break;
				default:
				break;
			}
		
			QCoreApplication::processEvents();
		}
		cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
	}

	delete checkDialog;
	checkDialog = NULL;
	ui->textEdit->setTextCursor(oldCursor);

	QMessageBox::information(this, tr("Finished"), tr("The spell check has finished."));
}

void Dialog::replaceAll(int nPos, const QString &sOld, const QString &sNew) 
{
	QTextCursor cursor(ui->textEdit->document());
	cursor.setPosition(nPos-sOld.length(), QTextCursor::MoveAnchor);

	while (!cursor.atEnd()) 
	{
		QCoreApplication::processEvents();
		cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
		QString word = cursor.selectedText();

		while (!word.isEmpty() && !word.at(0).isLetter() && cursor.anchor() < cursor.position())
		{
			int cursorPos = cursor.position();
			cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
			cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
			word = cursor.selectedText();
		}

		if (word == sOld) 
		{
			cursor.insertText(sNew);
			QCoreApplication::processEvents();
		}
		cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
	}
}
