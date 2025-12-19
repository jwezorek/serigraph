#include <QWidget>
#include <vector>
#include <array>
#include <cstdint>
#include <QColor>

// Forward decl needed for the vector
class QFrame;

namespace ser {

    // Defined based on your requirements
    using rgb_color = std::array<uint8_t, 3>;

    class palette_widget : public QWidget {
        Q_OBJECT

    public:
        explicit palette_widget(QWidget* parent = nullptr);

        void set_colors(const std::vector<QColor>& colors);
        std::vector<QColor> get_colors() const;
        void add_color(const QColor& color);
        void remove_swatch_at(int index);

    signals:
        void palette_changed();
        void color_added_requested(QColor);
        void color_delete_requested(int);

    private:
        void clear_layout();
        void insert_swatch(int index, const QColor& color);
        void contextMenuEvent(QContextMenuEvent* event);
        std::vector<QFrame*> swatches_;
    };

} // namespace sere ser