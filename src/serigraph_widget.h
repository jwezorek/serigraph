#pragma once
#include <QTabWidget>
#include <QWidget>
#include <QImage>
#include <QColor>

class QScrollArea; // Forward declaration

namespace ser {

    class serigraph_widget : public QTabWidget {
        Q_OBJECT

    public:
        explicit serigraph_widget(QWidget* parent = nullptr);

        void set_source_image(const QImage& image);
        void set_separated_image(const QImage& image);
        void set_reinked_image(const QImage& image);

        QImage src_image() const;

    signals:
        void source_pixel_clicked(QColor color);

    private:

        // Updates the scroll area behavior based on whether an image is loaded
        void update_scroll_behavior(QWidget* pane, QScrollArea* scroll, const QImage& img);

        // Pointers to the panes (the widgets drawing the images)
        QWidget* source_pane_;
        QWidget* separated_pane_;
        QWidget* reinked_pane_;

        // Pointers to the scroll areas (the containers inside the tabs)
        QScrollArea* source_scroll_;
        QScrollArea* separated_scroll_;
        QScrollArea* reinked_scroll_;
    };
}