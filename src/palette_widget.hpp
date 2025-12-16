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

        // New member function to hook up to your signal
        void add_color(const rgb_color& color);

    signals:
        void palette_changed();

    private:
        void clear_layout();

        // Helper to handle creation, insertion, and signal connections
        // for individual swatches (used by set_colors, add_color, and duplicate)
        void insert_swatch(int index, const QColor& color);

        // Storing QFrame* allows 'swatch' to be defined entirely in the .cpp
        std::vector<QFrame*> swatches_;
    };

} // namespace sere ser