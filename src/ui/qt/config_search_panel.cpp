#include "config_search_panel.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>

namespace yamy {
namespace ui {

ConfigSearchPanel::ConfigSearchPanel(QWidget *parent)
    : QWidget(parent), m_searchText(new QLineEdit(this)),
      m_filterType(new QComboBox(this)) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_searchText);
  layout->addWidget(m_filterType);

  m_searchText->setPlaceholderText(tr("Search..."));
  m_filterType->addItem(tr("Name"));
  m_filterType->addItem(tr("Tag"));
  m_filterType->addItem(tr("Status"));

  connect(m_searchText, &QLineEdit::textChanged, this, [this](const QString &text) {
    emit filterChanged(text, static_cast<FilterType>(m_filterType->currentIndex()));
  });

  connect(m_filterType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int index) {
            emit filterChanged(m_searchText->text(), static_cast<FilterType>(index));
          });
}

} // namespace ui
} // namespace yamy
