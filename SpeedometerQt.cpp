#include "SpeedometerQt.h"
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QMouseEvent>
#include <QPoint>
#include <QLabel>
#include <QFontDatabase>

#include <QGraphicsDropShadowEffect>


int WIDTH = 225;
int HEIGHT = WIDTH; // squared shape

int descr_label_pos_y = HEIGHT * 0.76;



RadialGauge::RadialGauge(QWidget* parent)
    : QMainWindow(parent)
{ 

    setWindowIcon(QIcon("gauge_icon.png"));
    build_UI();
   
    setWindowFlags(Qt::FramelessWindowHint);

    adjustSize();
    setFixedSize(size());
    connect_button_signals();  

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    setAttribute(Qt::WA_TranslucentBackground);
    //setStyleSheet("background:transparent;");

    run_demo_mode();
}

RadialGauge::~RadialGauge()
{}

void RadialGauge::create_close_button()
{
   
    // main_widget and gauges_layout have different dimensions - use size constants to position buttons
    m_close_button = new QPushButton("Exit", this);
    m_close_button->setFixedSize(width() * 0.05, height() * 0.04);
    //m_close_button->move(WIDTH * 2 - m_close_button->width() + 20, 10);// top-right
    m_close_button->move(WIDTH * 0.5 - m_close_button->width() * 0.2, HEIGHT * 0.3);

    m_close_button->setStyleSheet(
        "QPushButton {"
        "   background: transparent;"
        "   color: white;"
        "   border: 1px solid rgba(255,255,255,0.3);"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(255,255,255,0.1);"  // hover glow
        "}"
    );    

        
}



void RadialGauge::create_demo_button()
{
    m_demo_button = new QPushButton("Demo", this);

    // size as percentage of window
    m_demo_button->setFixedSize(width() * 0.07, height() * 0.04); 

    
    //m_demo_button->move(m_demo_button->width() - m_demo_button->width() * 0.7, 10);// top-right   
    m_demo_button->move(WIDTH * 0.5 - m_demo_button->width() * 0.30, HEIGHT * 0.6);  // upper middle of the first gauge 


    m_demo_button->setStyleSheet(
        "QPushButton {"
        "   background: transparent;"
        "   color: white;"
        "   border: 1px solid rgba(255,255,255,0.3);"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(255,255,255,0.1);"  // hover glow
        "}"
    );
   
}



//mouse events overrides
void RadialGauge::mousePressEvent(QMouseEvent* event) 
{
    if (event->button() == Qt::LeftButton) // if left button pressed
    {
        // get current mouse position and subtract the topleft corner position from it
        m_drag_position = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    
}

void RadialGauge::mouseMoveEvent(QMouseEvent* event) 
{
    if (event->buttons() & Qt::LeftButton)
    {
        // on drag - offset position from the mouse position to avoid jumps 
        move(event->globalPosition().toPoint() - m_drag_position);
        event->accept();
    }
}

void RadialGauge::build_UI()
{

    // create the master layout
    QWidget* central_widget = new QWidget(this);
    QVBoxLayout* master_layout = new QVBoxLayout(central_widget);

    m_gauges_area = new QHBoxLayout();
    //m_buttons_area = new QHBoxLayout();      
   
    m_system_monitor = new SystemMonitor(this);
    m_timer = new QTimer(this);

    create_cpu_number_output_label();

    create_cpu_gauge();

    create_memory_gauge(); 

    create_close_button();
    create_demo_button();
    

    m_timer->start(80); // Update every n milliseconds      

    master_layout->addLayout(m_gauges_area);
    //master_layout->addLayout(m_buttons_area, 1);

    master_layout->setAlignment(Qt::AlignTop);
    this->setCentralWidget(central_widget);      

}

void RadialGauge::create_cpu_number_output_label()
{
    // number output    

    int id = QFontDatabase::addApplicationFont("./digital-7 (mono).ttf");  // load the font
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);

    m_cpu_load_number = new QLabel(this);
    m_font = new QFont(family, 24, QFont::Bold);  // name, size
    m_cpu_load_number->setFont(*m_font); 
    //m_cpu_load_number->setText("000%");
    m_cpu_load_number->adjustSize();
    //m_font->setFixedPitch(true);
    m_cpu_load_number->move(WIDTH * 0.40, HEIGHT * 0.69);

    m_outline = new QGraphicsDropShadowEffect(this);
    m_outline->setBlurRadius(2);               // blur
    m_outline->setColor(QColor(0, 0, 0, 128)); // outline color
    m_outline->setOffset(3, 3);                // shadow offset
    m_outline->setXOffset(3);
    m_outline->setYOffset(3);
    m_cpu_load_number->setGraphicsEffect(m_outline);
    m_cpu_load_number->setStyleSheet("color: white;");  

    //qDebug() << QFontDatabase().families();
    
}

void RadialGauge::create_cpu_gauge()
{
    // CPU gauge
    m_cpu_gauge = new AnalogGauge(-110.0, 2.22, "./gauge_cpu.png", QString("CPU LOAD"), QPoint((WIDTH * 0.5) - (WIDTH / 6), descr_label_pos_y + HEIGHT * 0.03), this);
    m_cpu_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    m_gauges_area->addWidget(m_cpu_gauge);


    connect(m_timer, &QTimer::timeout, this, [this]()
        {
            double cpu = m_system_monitor->get_cpu_usage();
            //qDebug() << "CURRENT CPU LOAD: " << cpu;

            if (cpu < 1)
            {
                cpu = m_last_good_cpu_value; // keep previous value if current falls to zero to prevent jitter
            }
            else m_last_good_cpu_value = cpu;


            if (!m_paused)
            {
                m_cpu_gauge->set_speed(cpu);               

                int num = int(std::ceil(cpu));

                set_label_color(num);

                QString value_as_string = QString("%1").arg(num, 3, 10, QChar('0'));
                
                m_cpu_load_number->setText(QString("%1%").arg(value_as_string));
                m_cpu_load_number->adjustSize();
                m_font->setFixedPitch(true);
            }

                             

        });

}

void RadialGauge::set_label_color(int n)
{
    if (n < 40) m_cpu_load_number->setStyleSheet("color: #27C8F5;");
    else if (n < 80) m_cpu_load_number->setStyleSheet("color: #FFA000;");
    else m_cpu_load_number->setStyleSheet("color: red;");
}


void RadialGauge::create_memory_gauge()
{

    // memory gauge
    m_memory_gauge = new AnalogGauge(-149, 2.99, "./gauge_ram.png", QString(" RAM%\n in use"), QPoint((WIDTH * 0.5) - (WIDTH / 10), descr_label_pos_y), this);
    m_memory_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    m_gauges_area->addWidget(m_memory_gauge);
    
    connect(m_timer, &QTimer::timeout, this, [this]()
        {
            auto mem_perc_used = m_system_monitor->get_memory_usage();

            qDebug() << "MEMORY % USED: " << m_system_monitor->get_memory_usage();

            if (!m_paused)
            {
                m_memory_gauge->set_speed(mem_perc_used);
            }
        });

}



void RadialGauge::connect_button_signals()
{ 
  

   connect(m_demo_button, &QPushButton::clicked, this, [this](int value) { run_demo_mode();} );  

   connect(m_close_button, &QPushButton::clicked, this, &QMainWindow::close);

}


void RadialGauge::run_demo_mode()
{
    m_demo_button->setEnabled(false);
    
    m_cpu_gauge->move_needle();
    m_memory_gauge->move_needle();
    
    m_paused = true;

    QTimer::singleShot(2200, this, [this]()
        {
            
            double cpu = m_system_monitor->get_cpu_usage();
            double mem = m_system_monitor->get_memory_usage();

            m_cpu_gauge->animate_to(cpu);
            m_memory_gauge->animate_to(mem);

            QTimer::singleShot(500, this, [this]() 
                {
                    m_paused = false;
                    m_demo_button->setEnabled(true);
                });
        }
    );
}





int fudge = WIDTH * 0.15;
int arrow_length = WIDTH * 0.16;
int arrow_width = WIDTH * 0.015;
int cover_cap_radius = WIDTH * 0.04;


// GAUGE CLASS

AnalogGauge::AnalogGauge(double needle_start_pos, double max_range, const QString& bg, const QString& label_text, QPoint label_pos, QWidget* parent):
    m_current_angle(needle_start_pos), m_rotation_range(max_range), m_remap_value(needle_start_pos)
{   
    

    m_end_position = 100 * m_rotation_range + m_remap_value; // have the end position in a variable for the animations

    m_self_description_text = label_text;
    m_self_description_position = label_pos;

    load_background_image(bg);  
    
    set_needle_pivot();

    show_description_label();

    qDebug() << "Analog constructor";
}

void AnalogGauge::show_description_label()
{
    // test for glow effect
    m_self_description_label = new QLabel(m_self_description_text, this);
    m_self_description_label->setStyleSheet("color: white; font-size: 14px; font-weight: bold;"); // for now
    m_self_description_label->move(m_self_description_position);
    
    QGraphicsDropShadowEffect* glow = new QGraphicsDropShadowEffect();
    glow->setBlurRadius(25);
    glow->setOffset(0);
    glow->setColor(QColor(0, 255, 255, 200));  // glow color

    m_self_description_label->setGraphicsEffect(glow);
    m_self_description_label->show();
}

void AnalogGauge::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
   
    // draw background 
    painter.drawImage(rect(), m_background);     

    // transforms for needle
   
    painter.translate(m_gauge_center_x, m_gauge_center_y);  // move origin to gauge center
    painter.rotate(m_current_angle);         // rotate coordinate system

    // needle is drawn in transformed coordinates
    
    painter.drawImage((-m_needle_width / 2), -m_needle_height + fudge, m_needle);

   /* painter.setBrush(Qt::red);
    painter.drawEllipse(QPoint(0, 0 - fudge), arrow_width, arrow_length);*/

    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(0, 0), cover_cap_radius, cover_cap_radius);    
}

void AnalogGauge::set_needle_pivot()
{
    
    m_needle.load("./gauge_arrow.png");
    m_needle = m_needle.scaledToHeight(WIDTH * 0.5, Qt::SmoothTransformation);

    // needle pivot is set at the center of the circle
    m_gauge_center_x = WIDTH / 2;
    m_gauge_center_y = HEIGHT / 2;

    m_needle_width = m_needle.width();
    m_needle_height = m_needle.height();

    
}

void AnalogGauge::set_speed(double speed)
{
    this->m_current_angle = map_speed_to_angle(speed); 

    update(); // call repaint 

    //qDebug() << "Current angle" << m_current_angle;
}

double AnalogGauge::map_speed_to_angle(int speed) 
{   
    return speed * m_rotation_range + m_remap_value; // maps the range       
}



void AnalogGauge::move_needle()
{
    

    //m_main_button
    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    QPropertyAnimation* sweep = new QPropertyAnimation(this, "current_angle");
    int animation_duration = 1000;    

    int pause_duration = 100;
    
    sweep->setDuration(animation_duration); // animation length in miliseconds
    sweep->setStartValue(m_current_angle); // starting angle 

    

    sweep->setKeyValueAt(1, m_end_position);     // settle
    sweep->setEasingCurve(QEasingCurve::OutCubic);  // Smooth deceleration into settle

    // Return to start  
    QPropertyAnimation* retreat = new QPropertyAnimation(this, "current_angle");
    retreat->setEasingCurve(QEasingCurve::InOutQuad);
    retreat->setDuration(animation_duration);
    retreat->setStartValue(m_end_position);
    retreat->setEndValue(m_current_angle);

    group->addAnimation(sweep);
    group->addPause(100);
    group->addAnimation(retreat);
    group->start(QAbstractAnimation::DeleteWhenStopped); // clean up when done

   
}


void AnalogGauge::animate_to(double target_value)
{
    double target_angle = map_speed_to_angle(target_value);

    QPropertyAnimation* anim = new QPropertyAnimation(this, "current_angle");

    anim->setDuration(800);
    anim->setStartValue(m_current_angle);
    anim->setEndValue(target_angle);
    anim->setEasingCurve(QEasingCurve::OutCubic);

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}



// this is for the animation macro system from the header
void AnalogGauge::set_current_angle(double angle)
{
    if (m_current_angle != angle) // check if the value actually changed
    {         
        m_current_angle = angle;
        update(); // tells Qt to repaint the widget

        emit current_angle_changed(m_current_angle); 
    }

}

void AnalogGauge::load_background_image(const QString& bg)
{
    m_background.load(bg);
}



// MONITOR CLASS
// 
// system monitor class uses the Pdh header
// Declare a query - PDH_HQUERY my_query;
// Open the query - PdhCollectQueryData(my_query);
// Declare a counter for my_query - PDH_HCOUNTER my_counter;
// Set up the counter - PdhAddCounter(my_query, L"\\Processor(_Total)\\% Processor Time", 0, &my_counter);
// Get the recorded value in a usable format - PdhGetFormattedCounterValue(my_query, PDH_FMT_DOUBLE, NULL, &value);

SystemMonitor::SystemMonitor(QObject* parent): QObject(parent), m_smooth_cpu(0.0), m_smooth_disk(0.0)
{
    // Pdh queries
    PdhOpenQuery(NULL, 0, &m_cpu_query);
    PdhAddCounter(m_cpu_query, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpu_counter);

    PdhOpenQuery(NULL, 0, &m_memory_query);
    PdhAddCounter(m_memory_query, L"\\Memory\\Available MBytes", 0, &m_memory_counter);

    PdhOpenQuery(NULL, 0, &m_disk_query);
    PdhAddCounter(m_disk_query, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &m_disk_counter);// does not show anything useful

    
}

SystemMonitor::~SystemMonitor()
{
}

double SystemMonitor::get_cpu_usage()
{
    PdhCollectQueryData(m_cpu_query);
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(m_cpu_counter, PDH_FMT_DOUBLE, NULL, &value);

    double current_cpu = value.doubleValue;
    //qDebug() << "CURRENT CPU LOAD: " << current_cpu;    
    m_smooth_cpu = 0.9 * m_smooth_cpu + current_cpu * 0.1; // low pass
        
    return m_smooth_cpu;
}

double SystemMonitor::get_memory_usage()
{

    PdhCollectQueryData(m_memory_query);
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(m_memory_counter, PDH_FMT_DOUBLE, NULL, &value);

    // get total memory to calculate percentage
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&mem_info);

    DWORDLONG total_memory = mem_info.ullTotalPhys / (1024 * 1024); // convert to MB
    double available_MB = value.doubleValue;
    double used_memory_percentage = ((total_memory - available_MB) / total_memory) * 100.0;

    

    return used_memory_percentage;
}

double SystemMonitor::get_disk_read_speed()
{
    PdhCollectQueryData(m_disk_query);
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(m_disk_counter, PDH_FMT_DOUBLE, NULL, &value);

    double current_disk_usage = value.doubleValue;    

    m_smooth_disk = 0.9 * m_smooth_disk + current_disk_usage * 0.1;

    //qDebug() << "Cirrent Disk in Kb/s: " << current_disk_usage;

    return m_smooth_disk;

}


