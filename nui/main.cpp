#include <QApplication>
#include <QWidget>
#include <QString>
#include <QTimer>
#include <QPushButton>
#include <QGraphicsScene>
#include <QPainter>
#include <QThread>
#include <QMouseEvent>

#include "FileReader.hpp"
#include "Unlogger.hpp"

/*class Scrollbar : public QWidget {
  public:
    Scrollbar() {}
  protected:
    void paintEvent(QPaintEvent *event) override;
};

void Scrollbar::paintEvent(QPaintEvent *event) {
  printf("paint\n");
  //if (events.size() == 0) return;

  //QPainter painter(this);
}*/


class Window : public QWidget {
  public:
    Window(QString route_);
  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    uint64_t ct;
    Unlogger *unlogger;
  private:
    int timeToPixel(uint64_t ns);
    uint64_t pixelToTime(int px);
    QString route;

    // TODO: This is not thread safe
    Events events;
    QVector<LogReader*> lrs;
    QVector<FrameReader*> frs;
};

Window::Window(QString route_) : route(route_) {
  //QPushButton *quitBtn = new QPushButton("Quit", this);
  //quitBtn->setGeometry(50, 40, 75, 30);

  //Scrollbar *sb = new Scrollbar();
  //sb->setGeometry(0, 0, 200, 100);

  QThread* thread = new QThread;
  unlogger = new Unlogger(&events);
  unlogger->moveToThread(thread);
  connect(thread, SIGNAL (started()), unlogger, SLOT (process()));
  connect(unlogger, SIGNAL (elapsed()), this, SLOT (update()));
  thread->start();

  //for (int i = 0; i <= 6; i++) {
  for (int i = 2; i <= 2; i++) {
    QString fn = QString("%1/%2/rlog.bz2").arg(route).arg(i);
    lrs.append(new LogReader(fn, &events));
    /*QString frn = QString("%1/%2/fcamera.hevc").arg(route).arg(i);
    frs.append(new FrameReader(frn));*/
  }

}

int Window::timeToPixel(uint64_t ns) {
  // TODO: make this dynamic
  return int(ns*1e-9*4.0+0.5);
}

uint64_t Window::pixelToTime(int px) {
  // TODO: make this dynamic
  //printf("%d\n", px);
  return ((px+0.5)/4.0) * 1e9;
}

void Window::mousePressEvent(QMouseEvent *event) {
  //printf("mouse event\n");
  if (event->button() == Qt::LeftButton) {
    uint64_t t0 = events.begin().key();
    uint64_t tt = pixelToTime(event->x());
    //printf("seek to %lu\n", t0+tt);
    unlogger->setSeekRequest(t0+tt);
  }
  this->update();
}

void Window::paintEvent(QPaintEvent *event) {
  QElapsedTimer timer;
  timer.start();

  uint64_t t0 = events.begin().key();
  uint64_t t1 = (events.end()-1).key();

  QPainter p(this);

  p.setBrush(Qt::cyan);
  //p.drawRect(0, 0, 600, 100);

  // TODO: we really don't have to redraw this every time, only on updates to events
  int lt = -1;
  int lvv = 0;
  for (auto e : events) {
    auto type = e.which();
    //printf("%lld %d\n", e.getLogMonoTime()-t0, type);
    if (type == cereal::Event::CONTROLS_STATE) {
      auto controlsState = e.getControlsState();
      uint64_t t = (e.getLogMonoTime()-t0);
      float vEgo = controlsState.getVEgo();
      int enabled = controlsState.getState() == cereal::ControlsState::OpenpilotState::ENABLED;
      int rt = timeToPixel(t); // 250 ms per pixel
      if (rt != lt) {
        int vv = vEgo*10.0;
        if (lt != -1) {
          p.setPen(Qt::red);
          p.drawLine(lt, 300-lvv, rt, 300-vv);

          if (enabled) {
            p.setPen(Qt::green); 
          } else {
            p.setPen(Qt::blue); 
          }

          p.drawLine(rt, 300, rt, 600);
        }
        lt = rt;
        lvv = vv;
      }

      //printf("%f : %f\n", t, vEgo);
    }
  }
  uint64_t ct = unlogger->getCurrentTime();
  if (ct != 0) {
    int rrt = timeToPixel(ct-t0);
    p.drawRect(rrt-1, 0, 2, 600);
  }

  p.end();

  if (timer.elapsed() > 100) {
    qDebug() << "paint in" << timer.elapsed() << "ms";
  }
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QString route(argv[1]);
  if (route == "") {
    route = "3a5d6ac1c23e5536/2019-10-29--10-06-58";
    //route = "0006c839f32a6f99/2019-02-18--06-21-29";
    //route = "02ec6bea180a4d36/2019-10-25--10-18-09";
  }

  Window window(route);
  window.resize(1920, 600);
  window.setWindowTitle("nui unlogger");
  window.show();

  return app.exec();
}

