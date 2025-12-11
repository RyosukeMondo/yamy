#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;

namespace yamy {
namespace ui {

class ConfigSearchPanel : public QWidget {
  Q_OBJECT
public:
  enum class FilterType {
    Name,
    Tag,
    Status,
  };

  explicit ConfigSearchPanel(QWidget *parent = nullptr);

signals:
  void filterChanged(const QString &text, FilterType type);

private:
  QLineEdit *m_searchText;
  QComboBox *m_filterType;
};

} // namespace ui
} // namespace yamy
