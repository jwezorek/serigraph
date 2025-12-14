#pragma once
#include <QWidget>
#include <QImage>

class serigraph_widget : public QWidget {
    Q_OBJECT

public:
    explicit serigraph_widget(QWidget *parent = nullptr);
    
    // Call this when you load a file to update the view
    void set_source_image(const QImage &image);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
};