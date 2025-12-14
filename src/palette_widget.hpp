#include <QWidget>
#include <vector>
#include <QColor>

// Forward decl needed for the vector, but we use QFrame to decouple
class QFrame;

namespace ser {

    class palette_widget : public QWidget {
        Q_OBJECT

    public:
        explicit palette_widget(QWidget* parent = nullptr);

        void set_colors(const std::vector<QColor>& colors);
        std::vector<QColor> get_colors() const;

    signals:
        void palette_changed();

    private:
        void clear_layout();

        // Storing QFrame* allows 'swatch' to be defined entirely in the .cpp
        std::vector<QFrame*> swatches_;
    };

} // namespace ser