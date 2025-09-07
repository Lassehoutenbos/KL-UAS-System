#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QTimer>
#include <QPointF>
#include <QVector>
#include <QMenu>
#include <QAction>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>

class MapWidget : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapWidget(QWidget *parent = nullptr);
    
    void updateDronePosition(double lat, double lon, double heading);
    void addWaypoint(double lat, double lon);
    void clearWaypoints();
    void setHomePosition(double lat, double lon);
    void updateFlightPath(double lat, double lon);

signals:
    void waypointAdded(double lat, double lon);
    void waypointRemoved(int index);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void removeSelectedWaypoint();
    void clearAllWaypoints();
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    QGraphicsScene *mapScene;
    QGraphicsPixmapItem *mapBackground;
    QGraphicsEllipseItem *droneIcon;
    QGraphicsLineItem *droneHeading;
    QGraphicsEllipseItem *homeIcon;
    QGraphicsPathItem *flightPath;
    
    QVector<QGraphicsEllipseItem*> waypoints;
    QVector<QGraphicsLineItem*> waypointConnections;
    QVector<QPointF> waypointPositions;
    QVector<QPointF> flightPathPoints;
    
    // Map properties
    double centerLat;
    double centerLon;
    double mapScale;
    int selectedWaypoint;
    
    // Context menu
    QMenu *contextMenu;
    QAction *addWaypointAction;
    QAction *removeWaypointAction;
    QAction *clearWaypointsAction;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *resetZoomAction;
    
    void setupUI();
    void setupContextMenu();
    void createMapBackground();
    void updateWaypointConnections();
    QPointF latLonToScene(double lat, double lon);
    QPair<double, double> sceneToLatLon(QPointF point);
    void updateFlightPathDisplay();
};

#endif // MAPWIDGET_H
