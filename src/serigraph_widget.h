#pragma once
#include <QTabWidget>
#include <QWidget>
#include <QImage>
#include <QColor>

class QScrollArea; // Forward declaration
class ImagePane;   // Forward declaration

class serigraph_widget : public QTabWidget {
    Q_OBJECT

public:
    explicit serigraph_widget(QWidget* parent = nullptr);

    void set_source_image(const QImage& image);
    void set_separated_image(const QImage& image);
    void set_reinked_image(const QImage& image);

signals:
    void source_pixel_clicked(QColor color);

private:
    // Helper to create a tab with a scroll area
    ImagePane* create_tab(const QString& title, QScrollArea*& out_scroll_area);

    // Updates the scroll area behavior based on whether an image is loaded
    void update_scroll_behavior(ImagePane* pane, QScrollArea* scroll, const QImage& img);

    // Pointers to the panes (the widgets drawing the images)
    ImagePane* m_source_pane;
    ImagePane* m_separated_pane;
    ImagePane* m_reinked_pane;

    // Pointers to the scroll areas (the containers inside the tabs)
    QScrollArea* m_source_scroll;
    QScrollArea* m_separated_scroll;
    QScrollArea* m_reinked_scroll;
};