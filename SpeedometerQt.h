#pragma once
#pragma comment(lib, "Pdh.lib")
#include <Pdh.h>

#include <QtWidgets/QMainWindow>

class QSlider;
class QPushButton;
class QPaintEvent;
class AnalogGauge;
class QHBoxLayout;
class QVBoxLayout;
class QTimer;
class SystemMonitor;
class QPoint;
class QLabel;


class RadialGauge : public QMainWindow
{
    Q_OBJECT


    double m_main_slider_value = 0.0;

public slots:
    void connect_button_signals();
    void create_cpu_gauge();

    void create_memory_gauge();

    void create_disk_gauge();

    void run_demo_mode();

    void create_close_button();

    void create_demo_button();
    
public:
    RadialGauge(QWidget* parent = nullptr);
    ~RadialGauge();    
    void build_UI();

    void create_cpu_number_output_label();    

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    SystemMonitor* m_system_monitor;
    QHBoxLayout* m_gauges_area;
    QHBoxLayout* m_buttons_area;
    QTimer* m_timer;
    QLabel* m_cpu_load_number;
    QPoint m_drag_position;

    QSlider* m_main_slider1;    

    QPushButton* m_demo_button;
    QPushButton* m_close_button;
    AnalogGauge* m_cpu_gauge;  
    AnalogGauge* m_memory_gauge; 
    AnalogGauge* m_disk_gauge; 

    QWidget* m_click_area;
    

    double m_last_good_cpu_value = 1.0;

    bool m_paused = false;
};


class AnalogGauge : public QWidget
{
    Q_OBJECT    
        
    
    Q_PROPERTY(double current_angle READ get_current_angle WRITE set_current_angle NOTIFY current_angle_changed)

    //         data type property name  getter       setter                   signal
private:

    QImage m_background;
    QImage m_needle;


    int m_gauge_center_x;
    int m_gauge_center_y;

    int m_needle_width;
    int m_needle_height;   

    double m_rotation_range = 2.63; // max angle 
    double m_current_angle;
    double m_remap_value;

    double m_end_position;      
    

public:
    AnalogGauge(double needle_start_pos, double max_range, QString bg, QWidget* parent = nullptr); // max_range is about 3.6 for 360 degree rotation

    void paintEvent(QPaintEvent* event) override;

    void set_needle_pivot();

    void set_speed(double speed);

    double map_speed_to_angle(int speed);

    double get_gauge_end_position() const { return m_end_position; }

    void move_needle();   

    // for the animation
    double get_current_angle() const { return m_current_angle; }

    void set_current_angle(double angle);

    void load_bg(const QString& bg);

signals:
    void current_angle_changed(double new_angle);

    
    void animation_started();
    void animation_finished();

};
  

// system monitor
class SystemMonitor : public QObject
{
    Q_OBJECT

public:
    SystemMonitor(QObject*);
    ~SystemMonitor();

    double get_cpu_usage();
    double get_memory_usage();
    double get_disk_read_speed();
    

private:

    double m_smooth_cpu;
    double m_smooth_disk;

    PDH_HQUERY m_cpu_query;
    PDH_HCOUNTER m_cpu_counter;

    PDH_HQUERY m_memory_query;
    PDH_HCOUNTER m_memory_counter;

    PDH_HQUERY m_disk_query;
    PDH_HCOUNTER m_disk_counter;

};