#include "palette_widget.hpp"

#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QColorDialog>
#include <QFrame>
#include <QMenu>           // Added for Context Menu
#include <QContextMenuEvent> // Added for Context Menu
#include <array>           // Added for std::array

// Define the type locally to ensure compilation if not included from header
using rgb_color = std::array<uint8_t, 3>;

// -------------------------------------------------------------------------
// Internal Helper: swatch (Anonymous Namespace)
// -------------------------------------------------------------------------
namespace {

    class swatch : public QFrame {
        Q_OBJECT

    public:
        // Removed explicit 'index' argument; it is hard to maintain during dynamic list changes.
        // We will rely on the parent container to know the order.
        explicit swatch(const QColor& color, QWidget* parent = nullptr)
            : QFrame(parent), color_(color)
        {
            setFrameShape(QFrame::Box);
            setFrameShadow(QFrame::Plain);
            setLineWidth(1);
            setCursor(Qt::PointingHandCursor);
            setMinimumHeight(30);
        }

        QColor get_color() const { return color_; }

    signals:
        void color_changed();
        void delete_requested();
        void duplicate_requested();

    protected:
        void paintEvent(QPaintEvent* event) override {
            QFrame::paintEvent(event);

            QPainter painter(this);
            QRect r = contentsRect();
            painter.fillRect(r, color_);
        }

        void mousePressEvent(QMouseEvent* event) override {
            if (event->button() == Qt::LeftButton) {
                const QColor new_color = QColorDialog::getColor(
                    color_,
                    this,
                    "Select Ink Color",
                    QColorDialog::ShowAlphaChannel
                );

                if (new_color.isValid()) {
                    color_ = new_color;
                    update();
                    emit color_changed();
                }
            }
            // Pass other buttons up (e.g. Right Click)
            else {
                QFrame::mousePressEvent(event);
            }
        }

        // New: Handle Right-Click Context Menu
        void contextMenuEvent(QContextMenuEvent* event) override {
            QMenu menu(this);

            QAction* act_dup = menu.addAction("Duplicate");
            connect(act_dup, &QAction::triggered, this, &swatch::duplicate_requested);

            QAction* act_del = menu.addAction("Delete");
            connect(act_del, &QAction::triggered, this, &swatch::delete_requested);

            menu.exec(event->globalPos());
        }

    private:
        QColor color_;
    };

} // namespace


// -------------------------------------------------------------------------
// ser::palette_widget Implementation
// -------------------------------------------------------------------------

ser::palette_widget::palette_widget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addStretch();
}

// -------------------------------------------------------------------------
// Helper: Create and wire up a swatch
// -------------------------------------------------------------------------
// Note: This assumes this function is added as a private helper in the 
// class declaration, or we can use a lambda approach in set_colors/add_color. 
// For this implementation, I will inline the creation logic into a 
// method called 'insert_swatch' to avoid code duplication.
// -------------------------------------------------------------------------

void ser::palette_widget::insert_swatch(int index, const QColor& color) {
    auto* layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (!layout) return;

    // Instantiate the anonymous namespace class 'swatch'
    swatch* s = new swatch(color, this);

    // 1. Handle Color Change (Generic signal for the whole palette)
    connect(s, &swatch::color_changed, this, &ser::palette_widget::palette_changed);

    // 2. Handle Deletion
    connect(s, &swatch::delete_requested, this, [this, s]() {
        // Find s in our vector
        auto it = std::find(swatches_.begin(), swatches_.end(), s);
        if (it != swatches_.end()) {
            swatches_.erase(it);
            s->deleteLater(); // Removes from layout automatically
            emit palette_changed();
        }
        });

    // 3. Handle Duplication
    connect(s, &swatch::duplicate_requested, this, [this, s]() {
        auto it = std::find(swatches_.begin(), swatches_.end(), s);
        if (it != swatches_.end()) {
            // Calculate index for insertion
            std::size_t vec_idx = std::distance(swatches_.begin(), it);

            // Insert new swatch after the current one
            insert_swatch(static_cast<int>(vec_idx) + 1, s->get_color());
            emit palette_changed();
        }
        });

    // Determine layout insertion index.
    // If index is valid, insert there. Otherwise insert before the stretch.
    // Note: layout->count() includes the stretch item at the end.
    int layout_count = layout->count();

    // Safety clamp
    if (index < 0 || index > layout_count - 1) {
        index = layout_count - 1;
    }

    layout->insertWidget(index, s);

    // Update internal vector
    if (static_cast<size_t>(index) >= swatches_.size()) {
        swatches_.push_back(s);
    }
    else {
        swatches_.insert(swatches_.begin() + index, s);
    }
}

// -------------------------------------------------------------------------
// Public Interface
// -------------------------------------------------------------------------

void ser::palette_widget::set_colors(const std::vector<QColor>& colors) {
    clear_layout();

    // We don't need to manually manipulate the layout here because
    // insert_swatch handles insertion logic.
    // However, clear_layout leaves the stretch item, so we append 
    // to the end (index 0, 1, 2...).

    for (const auto& c : colors) {
        // Always insert at the end of the swatches list
        insert_swatch(static_cast<int>(swatches_.size()), c);
    }
}

void ser::palette_widget::add_color(const rgb_color& color) {
    QColor c(color[0], color[1], color[2]);
    // Insert at the end
    insert_swatch(static_cast<int>(swatches_.size()), c);
    emit palette_changed();
}

std::vector<QColor> ser::palette_widget::get_colors() const {
    std::vector<QColor> result;
    result.reserve(swatches_.size());

    for (const auto* frame : swatches_) {
        const auto* s = static_cast<const swatch*>(frame);
        result.push_back(s->get_color());
    }
    return result;
}

void ser::palette_widget::clear_layout() {
    QLayout* layout = this->layout();
    if (!layout) return;

    // Remove only swatches, leave the stretch item (which is usually the last item)
    // It is cleaner to just delete the widgets we track.

    for (QWidget* w : swatches_) {
        w->deleteLater();
    }
    swatches_.clear();
}

#include "palette_widget.moc"