/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2019, Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "icon-view-index-widget.h"
#include "icon-view-delegate.h"
#include "icon-view.h"

#include <QPainter>
#include <QPaintEvent>
#include <QLabel>

#include <QApplication>
#include <QStyle>
#include <QTextDocument>
#include <QFontMetrics>
#include <QScrollBar>

#include "file-info.h"
#include "file-item-proxy-filter-sort-model.h"
#include "file-item.h"

#include <QDebug>

using namespace Peony;
using namespace Peony::DirectoryView;

IconViewIndexWidget::IconViewIndexWidget(const IconViewDelegate *delegate, const QStyleOptionViewItem &option, const QModelIndex &index, QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);

    m_option = option;
    m_index = index;

    m_delegate = delegate;

    QSize size = delegate->sizeHint(option, index);
    setMinimumSize(size);

    //extra emblems
    auto proxy_model = static_cast<FileItemProxyFilterSortModel*>(delegate->getView()->model());
    auto item = proxy_model->itemFromIndex(index);
    if (item) {
        m_info = item->info();
    }

    m_delegate->initStyleOption(&m_option, m_index);
    m_option.features.setFlag(QStyleOptionViewItem::WrapText);
    m_option.textElideMode = Qt::ElideNone;

    auto opt = m_option;
    opt.rect.setHeight(9999);
    opt.rect.moveTo(0, 0);

    //qDebug()<<m_option.rect;
    auto iconExpectedSize = m_delegate->getView()->iconSize();
    QRect iconRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
    QRect textRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    auto y_delta = iconExpectedSize.height() - iconRect.height();
    opt.rect.moveTo(opt.rect.x(), opt.rect.y() + y_delta);
    if (opt.text.size() <= 10)
        setFixedHeight(iconExpectedSize.height() + textRect.height() + 20);

    m_option = opt;
}

void IconViewIndexWidget::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter p(this);
    //p.fillRect(0, 0, 999, 999, Qt::red);

    IconView *view = m_delegate->getView();
    auto visualRect = view->visualRect(m_index);
    this->move(visualRect.topLeft());

    //qDebug()<<m_option.backgroundBrush;
    //qDebug()<<this->size() << m_delegate->getView()->iconSize();

    auto opt = m_option;
    p.fillRect(opt.rect, m_delegate->selectedBrush());

    if (opt.text.size() > 10)
    {
        //use QTextEdit to show full file name when select
        m_edit = new QTextEdit();
        //QRect text_rect(0, m_delegate->getView()->iconSize().height(), this->size().width(), this->size().height());
        QRect text_rect(0, 0, this->size().width(), this->size().height());

        //different font and size, leave space for file icon, need to improve
        auto fm = new QFontMetrics(opt.font);
        //qDebug() << "font height:" << fm->height() << m_delegate->getView()->iconSize().height();
        QString show = "";
        int count = (m_delegate->getView()->iconSize().height() + 30)/fm->height();
        //small size font improve
        if (fm->height() <= 15)
            count = (m_delegate->getView()->iconSize().height() + 20)/fm->height();
        for (int i=0;i<count;i++)
        {
           show += "\r";
        }
        show += opt.text;
        m_edit->document()->setPlainText(show);

        m_edit->document()->setDefaultFont(opt.font);
        m_edit->document()->setTextWidth(this->size().width()-5);
        m_edit->setAlignment(Qt::AlignCenter);
        m_edit->document()->drawContents(&p);
        //set color is ineffective, FIX ME
        //m_edit->setTextColor(QColor(255,255,255));
        m_edit->adjustSize();
        setFixedHeight(int(m_edit->document()->size().height()));

        //text already draw by doc, clear text
        opt.text = "";
        delete m_edit;
    }

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, &p, opt.widget);
    //qDebug() << "visualRect:" << visualRect << "text:" << opt.text;

    //extra emblems
    if (!m_info.lock()) {
        return;
    }
    auto info = m_info.lock();
    //paint symbolic link emblems
    if (info->isSymbolLink()) {
        QIcon icon = QIcon::fromTheme("emblem-symbolic-link");
        //qDebug()<< "symbolic:" << info->symbolicIconName();
        icon.paint(&p, this->width() - 30, 10, 20, 20, Qt::AlignCenter);
    }

    //paint access emblems
    //NOTE: we can not query the file attribute in smb:///(samba) and network:///.
    if (!info->uri().startsWith("file:")) {
        return;
    }

    auto rect = this->rect();
    if (!info->canRead()) {
        QIcon icon = QIcon::fromTheme("emblem-unreadable");
        icon.paint(&p, rect.x() + 10, rect.y() + 10, 20, 20);
    } else if (!info->canWrite() && !info->canExecute()){
        QIcon icon = QIcon::fromTheme("emblem-readonly");
        icon.paint(&p, rect.x() + 10, rect.y() + 10, 20, 20);
    }
}