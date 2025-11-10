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
class QFont;
class QPoint;
class QCheckBox;
class QGraphicsDropShadowEffect;


class RadialGauge : public QMainWindow
{
    Q_OBJECT
    
public slots:
    void connect_button_signals();
    void create_cpu_gauge();

    void set_label_color(int n);

    void create_memory_gauge();

    //void create_disk_gauge();

    void run_demo_mode();    

    void create_close_button();

    void create_demo_button();

    void set_always_on_top(bool enabled);
    
public:
    RadialGauge(QWidget* parent = nullptr);
    ~RadialGauge();    
    void build_UI();

    void create_cpu_number_output_label();

    void create_checkbox();

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

    QFont* m_font;
    QGraphicsDropShadowEffect* m_outline;

    QPushButton* m_demo_button;
    QPushButton* m_close_button;
    QCheckBox* m_on_top_checkbox;

    AnalogGauge* m_cpu_gauge;  
    AnalogGauge* m_memory_gauge; 
    AnalogGauge* m_disk_gauge;    

    double m_last_good_cpu_value = 1.0;

    bool m_paused = true;
};


class AnalogGauge : public QWidget
{
    Q_OBJECT    
        
    
    Q_PROPERTY(double current_angle READ get_current_angle WRITE set_current_angle NOTIFY current_angle_changed)

    //      data type property name      getter                  setter                   signal
private:

    QImage m_background;
    QImage m_needle;
    QImage m_needle_cap;

    QLabel* m_self_description_label;

    double m_current_angle;
    double m_rotation_range; // max angle     
    double m_remap_value;

    QString m_self_description_text;
    QPoint m_self_description_position;

    double m_end_position;
    int m_needle_pivot_offset;

    int m_gauge_center_x;
    int m_gauge_center_y;

    int m_needle_width;
    int m_needle_height;

    int m_needle_cap_width;   
    int m_needle_cap_height;        

public:
    AnalogGauge(
        double needle_start_pos,
        double max_range,
        const QString& bg,
        const QString& needle_img,
        const QString& neddle_cap_img,
        const QString& label_text,
        QPoint label_pos,
        QWidget* parent = nullptr
    ); 
    // max_range is 3.6 for 360 degree rotation

    void show_description_label();

    void paintEvent(QPaintEvent* event) override;

    void set_needle_pivot();

    void set_speed(double speed);

    double map_speed_to_angle(int speed);

    double get_gauge_end_position() const { return m_end_position; }

    void move_needle();

    void animate_to(double target_value);//

    // for the animation
    double get_current_angle() const { return m_current_angle; }

    void set_current_angle(double angle);

    void load_background_image(const QString& bg);

    void load_needle_image(const QString& needle_image);

    void load_needle_cap_image(const QString& needle_cap_image);

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
    explicit SystemMonitor(QObject* parent = nullptr);
    ~SystemMonitor();

   

    double get_cpu();
    double get_memory();
    

private:

    #ifdef Q_OS_WIN
   

    void init_pdh_queries();
    double get_cpu_usage_pdh();
    double get_memory_usage_pdh();    

    double m_smooth_cpu;
    double m_smooth_disk;

    PDH_HQUERY m_cpu_query;
    PDH_HCOUNTER m_cpu_counter;

    PDH_HQUERY m_memory_query;
    PDH_HCOUNTER m_memory_counter; 

    
    // Linux implementation

    #elif defined(Q_OS_LINUX)  


    double m_smooth_cpu;
    double m_smooth_disk;

    double query_cpu_proc();  
    double query_ram_proc();

    double get_cpu_usage();
    double get_ram_usage();


    #endif

};