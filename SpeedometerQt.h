#pragma once

#include <QtWidgets/QMainWindow>

class QSlider;
class QPushButton;
class QPaintEvent;
class AnalogGauge;

class RadialGauge : public QMainWindow
{
    Q_OBJECT


    double m_main_slider_value = 0.0;

public slots:
    void connect_signals();
    
    
public:
    RadialGauge(QWidget* parent = nullptr);
    ~RadialGauge();    
    void build_UI();

    

private:

    QSlider* m_main_slider1;
    QSlider* m_main_slider2;
    QSlider* m_main_slider3;

    QPushButton* m_main_button;
    AnalogGauge* m_speed_gauge_1;  
    AnalogGauge* m_speed_gauge_2;  
    AnalogGauge* m_speed_gauge_3;  
    AnalogGauge* m_speed_gauge_4;  
    AnalogGauge* m_speed_gauge_5;  
    AnalogGauge* m_speed_gauge_6;  

    
};


class AnalogGauge : public QWidget
{
    Q_OBJECT    
        
    
    Q_PROPERTY(double current_angle READ current_angle WRITE set_current_angle NOTIFY current_angle_changed)

    //property               data type   property name  getter       setter        signal
private:

    QImage m_background;
    QImage m_needle;

    int m_center_x;
    int m_center_y;

    int m_needle_width;
    int m_needle_height;   

    double m_rotation_range = 2.63; // max angle 
    double m_current_angle;
    double m_remap_value;
    

public:
    AnalogGauge(double needle_start_pos, double max_range, QString bg, QWidget* parent = nullptr); // max_range is about 3.6 for 360 degree rotation

    void paintEvent(QPaintEvent* event) override;

    void load_assets();

    void set_speed(double speed);

    double map_speed_to_angle(int speed);

    void move_needle();   

    // for the animation
    double current_angle() const { return m_current_angle; }

    void set_current_angle(double angle);

    void load_bg(const QString& bg);

signals:
    void current_angle_changed(double new_angle);


    void animation_started();
    void animation_finished();

};
    