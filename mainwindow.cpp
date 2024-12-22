#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QScrollArea>
#include<QPixmap>
#include<QLabel>
#include<QVBoxLayout>
#include<QTableWidget>
#include<QHeaderView>
#include<QTableWidgetItem>
#include<QDialog>
#include<QDockWidget>
#include<QGraphicsScene>
#include<QGraphicsView>
#include"ort.h"

#include <algorithm>  // für std::sort
#include <utility>

#include<QInputDialog>
#include<QMessageBox>
#include<sstream>
#include<fstream>
#include<cmath>

#include"adresse.h"
#include"poi.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
     ,ui(new Ui::MainWindow)
{
       ui->setupUi(this);
    // Create scroll area

       scrollArea = new QScrollArea(this);
       mapContainer = new QWidget();





    QPixmap pix(":/Germany_map/866-8664209_leer-deutschland-karte.png");

QLabel* mapLabel = new QLabel(mapContainer);

    mapLabel->setPixmap(pix);
    mapLabel->setGeometry(0,0,pix.width(),pix.height());
    mapLabel->setScaledContents(true);
    mapLabel->setMaximumSize(pix.size());
    mapLabel->setAlignment(Qt::AlignCenter);



    mapContainer->setFixedSize(pix.width(),pix.height());

    scrollArea->setWidget(mapContainer);
    scrollArea->setWidgetResizable(true);


    pixW=pix.width();
    pixH=pix.height();


 setCentralWidget(scrollArea);
    QWidget* buttonContainer = new QWidget(this);
    QVBoxLayout* buttonLayout = new QVBoxLayout(buttonContainer);

    buttonLayout->addWidget(ui->pushButton_Entfernung);
    buttonLayout->addWidget(ui->pushButton_information);
    buttonLayout->addWidget(ui->pushButton_alle_orten);
    buttonLayout->addWidget(ui->pushButton_Mein_Ort);
    buttonLayout->addWidget(ui->pushButton_Verschieben);
    buttonLayout->addWidget(ui->pushButton_ortLoschen);
     buttonLayout->addWidget(ui->pushButton_routing);

   // connect(ui->pushButton_ortLoschen,&QPushButton::clicked,this,&MainWindow::on_pushButton_ortLoschen_clicked());



    connect(ui->actionbeenden, &QAction::triggered, this, &MainWindow::onBeendenClicked);

    QDockWidget* buttonDock = new QDockWidget(this);
    buttonDock->setWidget(buttonContainer);
    buttonDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::RightDockWidgetArea, buttonDock);

   scene = new QGraphicsScene(mapContainer);
   QGraphicsView* view = new QGraphicsView(scene, mapContainer);
  view->setGeometry(mapContainer->geometry());
  view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::onBeendenClicked(){
    QApplication::quit();
}



void MainWindow::connectToNearestNeighbors(Ort *newOrt, int numNeighbors)
{
    std::vector<std::pair<double, Ort*>> neighbors;

    // Find all potential neighbors
    for(auto& existingOrt : MeinOrt) {
        if(existingOrt != newOrt && existingOrt != myPosition) {
            double distance = Rechnung(*newOrt, *existingOrt);
            neighbors.push_back({distance, existingOrt});
        }
    }

    // Sort by distance
    std::sort(neighbors.begin(), neighbors.end());

    // Connect to nearest neighbors
    int connectCount = std::min(numNeighbors, static_cast<int>(neighbors.size()));
    for(int i = 0; i < connectCount; i++) {
        newOrt->setNachbar(neighbors[i].second);
        neighbors[i].second->setNachbar(newOrt);
    }

    drawConnections();
}

void MainWindow::deleteOrt(Ort *ort)
{

    // if(!ort || ort == myPosition) return;

    // ort->setActive(false);

    // // Remove connections to this location
    // for(auto& other : MeinOrt) {
    //     if(other && other != ort) {
    //         std::vector<Ort*> nachbarn = other->getNachbarn();
    //         auto it = std::find(nachbarn.begin(), nachbarn.end(), ort);
    //         if(it != nachbarn.end()) {
    //             nachbarn.erase(it);
    //         }
    //     }
    // }

    // // Remove from MeinOrt
    // auto it = std::find(MeinOrt.begin(), MeinOrt.end(), ort);
    // if(it != MeinOrt.end()) {
    //     MeinOrt.erase(it);
    // }

    // for(auto buttonIt = MeinButton.begin(); buttonIt != MeinButton.end(); ++buttonIt) {
    //     if((*buttonIt)->text() == QString::fromStdString(ort->getName())) {
    //         (*buttonIt)->setVisible(false);  // Statt löschen nur ausblenden
    //         break;
    //     }
    // }

    // // Delete the Ort object
    // delete ort;

    // // Update connections and display
    // drawConnections();
    if(!ort || ort == myPosition) return;

    // Option 1: Logisches Löschen
    ort->setActive(false);

    // Entferne Verbindungen zu inaktiven Orten
    for(auto& other : MeinOrt) {
        if(other && other->isActive()) {
             std::vector<Ort*> nachbarn = other->getNachbarn();
            nachbarn.erase(
                std::remove_if(nachbarn.begin(), nachbarn.end(),
                               [](Ort* n) { return !n->isActive(); }),
                nachbarn.end()
                );
        }
    }

    // UI aktualisieren
    for(auto buttonIt = MeinButton.begin(); buttonIt != MeinButton.end(); ++buttonIt) {
        if((*buttonIt)->text() == QString::fromStdString(ort->getName())) {
            (*buttonIt)->hide(); // Statt löschen nur ausblenden
            break;
        }
    }

    // Verbindungen neu zeichnen
    drawConnections();

    // Optional: Nachbarn neu verbinden
    for(auto& other : MeinOrt) {
        if(other->isActive() && other != myPosition) {
            connectToNearestNeighbors(other);
        }
    }
}

void MainWindow::findRoute(Ort *start, Ort *end)
{
    if(!start->isActive() || !end->isActive()) {
        QMessageBox::warning(this, "Fehler", "Start- oder Zielort ist nicht aktiv.");
        return;
    }
    std::map<Ort*, RouteNode> nodes;
    for(auto& ort : MeinOrt) {
        if(ort->isActive()){
        nodes[ort].ort = ort;
        }
    }
    nodes[start].distance = 0;
        // Dijkstra's algorithm
    while(true) {
        // Find unvisited node with minimum distance
        Ort* current = nullptr;
        double minDist = std::numeric_limits<double>::max();
        for(auto& pair : nodes) {
            if(!pair.second.visited && pair.second.distance < minDist) {
                current = pair.first;
                minDist = pair.second.distance;
            }
        }
        if(!current || current == end) break;
        nodes[current].visited = true;
        // Update distances to neighbors
        for(auto& neighbor : current->getNachbarn()) {
            if(!nodes[neighbor].visited) {
                double newDist = nodes[current].distance + Rechnung(*current, *neighbor);
                if(newDist < nodes[neighbor].distance) {
                    nodes[neighbor].distance = newDist;
                    nodes[neighbor].previous = current;
                }
            }
        }
    }
    std::vector<Ort*> route;
    Ort* current = end;
    while(current) {
        route.push_back(current);
        current = nodes[current].previous;
    }
    std::reverse(route.begin(), route.end());
        // Display route information
    QString routeInfo = "Route:\n";
    double totalDistance = 0;

    clearRouteDisplay();
    QPen routePen(Qt::blue, 3); // Blue lines for the route
    for(size_t i = 0; i < route.size() - 1; i++) {
        double segmentDist = Rechnung(*route[i], *route[i + 1]);
        totalDistance += segmentDist;
        routeInfo += QString::fromStdString(route[i]->getName()) +
                     " -> " + QString::fromStdString(route[i + 1]->getName()) +
                     QString(" (%1 km)\n").arg(segmentDist);
        // Draw route segment
        int x1 = gpsToPixelX(route[i]->getLaengengrad());
        int y1 = gpsToPixelY(route[i]->getBreitengrad());
        int x2 = gpsToPixelX(route[i + 1]->getLaengengrad());
        int y2 = gpsToPixelY(route[i + 1]->getBreitengrad());

        QLineF linef(x1, y1, x2, y2);
        QGraphicsLineItem* line = scene->addLine(linef, routePen);
        routeLines.push_back(line);
    }
    routeInfo += QString("\nGesamtentfernung: %1 km").arg(totalDistance);
    QMessageBox::information(this, "Route Information", routeInfo);
}

bool MainWindow::areConnected(Ort *ort1, Ort *ort2)
{
    if (!ort1 || !ort2) return false;

    const auto& nachbarn = ort1->getNachbarn();
    return std::find(nachbarn.begin(), nachbarn.end(), ort2) != nachbarn.end();
}

    void MainWindow::clearRouteDisplay() {
        for(auto line : routeLines) {
            scene->removeItem(line);
            delete line;
        }
        routeLines.clear();
    }



int MainWindow::FindOrt(const string Name)
{
    bool found=false;
    int value=-1;//nicht gefunden
    for(unsigned int i =0; i<MeinOrt.size();i++)
    {
        if(MeinOrt[i]->getName()==Name)
        {
            found=true;
            value=i;
            return value;
        }
    }
    if(!found)
        return value;

    return value;
}

Ort *MainWindow::findNearestNeighbor(const std::vector<Ort *> &orte, Ort *newOrt)
{

    if (orte.size() < 1) return nullptr;//leer oder nicht

    double minDistance = std::numeric_limits<double>::max();
    Ort* nearest = nullptr;

    for (auto& ort : orte) {
        // Überspringe den neuen Ort selbst
        if (ort == newOrt) continue;


        double distance = Rechnung(*newOrt, *ort);
        if (distance < minDistance) {
            minDistance = distance;
            nearest = ort;
        }
    }

    return nearest;
}

double MainWindow::Rechnung(const Ort &ort1, const Ort &ort2)
{
    const double R=6371.0; //Erdradius
    double Laengen1 = Convert(ort1.getLaengengrad());
    double Breiten1 = Convert(ort1.getBreitengrad());
    double Laengen2 = Convert(ort2.getLaengengrad());
    double Breiten2 = Convert(ort2.getBreitengrad());


    double DeltaLaengen = Laengen2 - Laengen1;
    double DeltaBreiten = Breiten2 - Breiten1;

    // double a= sin(DeltaBreiten/2)*sin(DeltaBreiten)+cos(Breiten1)*cos(Breiten2)*sin(DeltaLaengen/2)*sin(DeltaLaengen);



    // double c= 2*atan2(sqrt(a),sqrt(1-a));

    // double result= R*c;
    double a = sin(DeltaBreiten / 2) * sin(DeltaBreiten / 2) +
               cos(Breiten1) * cos(Breiten2) * sin(DeltaLaengen / 2) * sin(DeltaLaengen / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    // Calcul final de la distance
    double result = R * c;


    return result;
}

double MainWindow::Convert(double value)
{
    return value*M_PI/180.0;
}

void MainWindow::Information( Ort *ort)
{

    QDialog dialog(this);
    dialog.setWindowTitle("Navigationssystem - Orte anzeigen");
    dialog.resize(1200, 600);
        // QTableWidget erstellen
    QTableWidget* table = new QTableWidget(&dialog);
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID", "Typ", "Name", "Breitengrad", "Laengengrad", "Parameter", "Nachbarn"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->insertRow(0);
    QString typ;
    QString parameters;
    QString nachbarnInfo = "Keine Nachbarn";
        // Bestimme Typ und Parameter
    if(PoI* poi = dynamic_cast<PoI*>(ort))
    {
        typ = "PoI";
        parameters = QString::fromStdString(poi->getKategorie()) + ", " +
                     QString::fromStdString(poi->getBemerkung());
    }
    else if(Adresse* adresse = dynamic_cast<Adresse*>(ort))
    {
        typ = "Adr";
        parameters = QString::fromStdString(adresse->getStrasse()) + ", " +
                     QString::number(adresse->getHausNummer()) + ", " +
                     QString::number(adresse->getPLZ()) + ", " +
                     QString::fromStdString(adresse->getGemeinde());
    }
    else
    {
        typ = "Unbekannt";
        parameters = "Keine weiteren Parameter";
    }
    // Erstelle Nachbarinformationen mit Entfernungen
    if(!ort->getNachbarn().empty()) {
        nachbarnInfo = "";
        for(const auto& nachbar : ort->getNachbarn()) {
            if(!nachbarnInfo.isEmpty()) nachbarnInfo += "\n";
            double distance = Rechnung(*ort, *nachbar);
            nachbarnInfo += QString::fromStdString(nachbar->getName()) +
                            " (ID: " + QString::number(nachbar->getID()) +
                            ", Entfernung: " + QString::number(distance, 'f', 2) + " km)";
        }
    }
    // Fülle Tabelle
    table->setItem(0, 0, new QTableWidgetItem(QString::number(ort->getID())));
    table->setItem(0, 1, new QTableWidgetItem(typ));
    table->setItem(0, 2, new QTableWidgetItem(QString::fromStdString(ort->getName())));
    table->setItem(0, 3, new QTableWidgetItem(QString::number(ort->getBreitengrad())));
    table->setItem(0, 4, new QTableWidgetItem(QString::number(ort->getLaengengrad())));
    table->setItem(0, 5, new QTableWidgetItem(parameters));
    table->setItem(0, 6, new QTableWidgetItem(nachbarnInfo));
        // Passe Zeilenhöhe an für mehrere Nachbarn
    table->resizeRowToContents(0);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->addWidget(table);
    dialog.setLayout(layout);
    dialog.exec();


}

void MainWindow::SchiebenExistierende()
{
    bool ok;
    QString nameString  = QInputDialog::getText(this, "Ort Name eingeben", "Geben Sie ein Ort Name an:", QLineEdit::Normal,"" ,&ok);
    if (!ok) return;
    string name=nameString.toStdString();
    if(FindOrt(name)!=0&& FindOrt(name)!=-1)
    {
        if(Adresse* adresse=dynamic_cast<Adresse*>(MeinOrt[FindOrt(name)]))
        {


            MeinOrt[0]=new Adresse(adresse->getLaengengrad(),adresse->getBreitengrad(),'a',0,
                                     adresse->getStrasse(),adresse->getHausNummer(),adresse->getPLZ(),adresse->getGemeinde(),adresse->getName());

            QMessageBox::information(this,"Erfolg","MeinOrt erfolgreich geschoben");
        }
        else if(PoI* poi=dynamic_cast<PoI*>(MeinOrt[FindOrt(name)]))
        {


            MeinOrt[0]=new PoI(poi->getLaengengrad(),poi->getBreitengrad(),'p',0,poi->getKategorie(),poi->getBemerkung(),poi->getName());
            QMessageBox::information(this,"Erfolg","MeinOrt erfolgreich geschoben");
        }

    }
    else
        QMessageBox::information(this,"ERROR","Mein Ort kann nicht verschoben sein");
}

void MainWindow::SchiebenNewOrt()
{

    bool ok; // speichert, ob der Benutzer eine gültige Auswahl getroffen hat.
    QStringList ortstypen = {"Adresse", "Point of Interest"};
    QString typ = QInputDialog::getItem(this, "Ortstyp wählen", "Wählen Sie den Typ des Ortes:", ortstypen, 0, false, &ok);
    if (!ok) return;  // Abbruch
    // Eingabe der gemeinsamen Eigenschaften
    double laengengrad = QInputDialog::getDouble(this, "Längengrad eingeben", "Geben Sie den Längengrad ein (-180 bis 180):", 0.0, -180, 180, 6, &ok);
    if (!ok) return;  // Abbruch

    double breitengrad = QInputDialog::getDouble(this, "Breitengrad eingeben", "Geben Sie den Breitengrad ein (-90 bis 90):", 0.0, -90, 90, 6, &ok);
    if (!ok) return;  // Abbruch

    QString name = QInputDialog::getText(this, "Name eingeben", "Geben Sie den Namen des Ortes ein:", QLineEdit::Normal, "", &ok);
    if (!ok) return;  // Abbruch

    // Adresse erstellen
    if (typ == "Adresse") {
        QString strasse = QInputDialog::getText(this, "Straße eingeben", "Geben Sie die Straße ein:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        int hausnummer = QInputDialog::getInt(this, "Hausnummer eingeben", "Geben Sie die Hausnummer ein:", 1, 1, 10000, 1, &ok);
        if (!ok) return;

        int postleitzahl = QInputDialog::getInt(this, "Postleitzahl eingeben", "Geben Sie die Postleitzahl ein:", 10000, 10000, 99999, 1, &ok);
        if (!ok) return;

        QString gemeinde = QInputDialog::getText(this, "Gemeinde eingeben", "Geben Sie die Gemeinde ein:", QLineEdit::Normal, "", &ok);
        if (!ok) return;
        //QString gemeinde = "hochschule";
        // Adresse erstellen und speichern
        string nameString=name.toStdString();
        string strassString=strasse.toStdString();
        string gemeindeString=gemeinde.toStdString();

        MeinOrt[0]=new Adresse(laengengrad,breitengrad,'a',0,strassString,hausnummer,postleitzahl,gemeindeString,nameString);
        AddRadioButton(nameString,breitengrad,laengengrad);
        QMessageBox::information(this, "Erfolg", "Die Adresse wurde erfolgreich angelegt!");
    }
    // POI erstellen
    else if (typ == "Point of Interest") {
        QString bemerkung = QInputDialog::getText(this, "Bemerkung eingeben", "Geben Sie eine Bemerkung ein:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        QString kategorie = QInputDialog::getText(this, "Kategorie eingeben", "Geben Sie die Kategorie ein (z.B. Restaurant, Tankstelle):", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        // POI erstellen und speichern
        string bemerkungString=bemerkung.toStdString();
        string kategorieString=kategorie.toStdString();
        string nameString=name.toStdString();

        MeinOrt[0]=new PoI(laengengrad,breitengrad,'p',0,kategorieString,bemerkungString,nameString);
        AddRadioButton(nameString,breitengrad,laengengrad);
        QMessageBox::information(this, "Erfolg", "Der Point of Interest wurde erfolgreich angelegt!");
    }
}

int MainWindow::gpsToPixelX(double gpsX)
{
    //const int pixelWidth= ui->label->width();
return ((gpsX - gpsTopLeftX) / (gpsBottomRightX - gpsTopLeftX)) * pixW;
}

int MainWindow::gpsToPixelY(double gpsY)
{
   // const int pixelHeight=ui->label->height();
   // QMessageBox::information(this,"test",QString::number(pixelHeight));
    return ((gpsY - gpsTopLeftY) / (gpsBottomRightY - gpsTopLeftY)) * pixH;
}

void MainWindow::drawConnections()
{

    // Bestehende Linien löschen
    for(auto line : connectionLines) {
        scene->removeItem(line);
        delete line;
    }
    connectionLines.clear();

    // Neue Verbindungen zeichnen


    QPen pen(Qt::gray, 2); // Red lines, 2px width

    for(auto& ort : MeinOrt) {
        if(ort && !ort->getNachbarn().empty()) {
            for(auto& nachbar : ort->getNachbarn()) {
                // Convert GPS coordinates to pixel coordinates
                int x1 = gpsToPixelX(ort->getLaengengrad());
                int y1 = gpsToPixelY(ort->getBreitengrad());
                int x2 = gpsToPixelX(nachbar->getLaengengrad());
                int y2 = gpsToPixelY(nachbar->getBreitengrad());
                    // Create line
                QLineF linef(x1, y1, x2, y2);
                QGraphicsLineItem* line = scene->addLine(linef, pen);
                connectionLines.push_back(line);
            }
        }
    }

}




void MainWindow::AddRadioButton(const string &name, const double &Breitengrad, const double Laengengrad)
{
    QString NameString=QString::fromStdString(name);

    QRadioButton* radio=new QRadioButton(mapContainer);
    radio->setText(NameString);
    int w = gpsToPixelX(Laengengrad);//W=X
    int h = gpsToPixelY(Breitengrad);
    radio->setAutoExclusive(false);

    // Find nearest neighbor
    Ort* current = nullptr;
    for (auto& ort : MeinOrt) {
        if (ort->getName() == name) {
            current = ort;
            break;
        }
    }

    if(current && !current->getNachbarn().empty()) {
        // Erstelle Tooltip mit Nachbarinformationen
        QString tooltip = "Nächste Nachbarn:\n";
        for(auto& nachbar : current->getNachbarn()) {
            tooltip += QString::fromStdString(nachbar->getName()) +
                       " (ID: " + QString::number(nachbar->getID()) + ")\n";
        }
        radio->setToolTip(tooltip);
    }

    MeinButton.push_back(radio);
    radio->move(w,h);
    radio->show();
}

void MainWindow::showPosition(const double &Breitengrad, const double Laengengrad)
{
    if(MeinLabel.size()==1)
    {
        MeinLabel[0]->hide();
        delete MeinLabel[0];
        MeinLabel.clear();
    }
   int  x=gpsToPixelX(Laengengrad);
     int y=gpsToPixelY(Breitengrad);
     QLabel* marker = new QLabel(mapContainer);
    marker->setStyleSheet("background-color: red; color: white; border: 1px solid black;");
    marker->setToolTip("Mein Standort");
    marker->setFixedSize(10, 10);
    marker->move(x , y);
    MeinLabel.push_back(marker);
    marker->show();
}

void MainWindow::on_action_1_Ort_Anlegen_triggered()
{
    if(MeinOrt.size()==0)
    {
        MeinOrt.push_back(myPosition);
    }

    // Typ des Ortes auswählen
    bool ok; // speichert, ob der Benutzer eine gültige Auswahl getroffen hat.
    QStringList ortstypen = {"Adresse", "Point of Interest"};
    QString typ = QInputDialog::getItem(this, "Ortstyp wählen", "Wählen Sie den Typ des Ortes:", ortstypen, 0, false, &ok);
    if (!ok) return;  // Abbruch

    // Eingabe der gemeinsamen Eigenschaften
    double laengengrad = QInputDialog::getDouble(this, "Längengrad eingeben", "Geben Sie den Längengrad ein (-180 bis 180):", 0.0, -180, 180, 6, &ok);
    if (!ok) return;  // Abbruch

    double breitengrad = QInputDialog::getDouble(this, "Breitengrad eingeben", "Geben Sie den Breitengrad ein (-90 bis 90):", 0.0, -90, 90, 6, &ok);
    if (!ok) return;  // Abbruch

    QString name = QInputDialog::getText(this, "Name eingeben", "Geben Sie den Namen des Ortes ein:", QLineEdit::Normal, "", &ok);
    if (!ok) return;  // Abbruch

    // Adresse erstellen
    if (typ == "Adresse") {
        QString strasse = QInputDialog::getText(this, "Straße eingeben", "Geben Sie die Straße ein:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        int hausnummer = QInputDialog::getInt(this, "Hausnummer eingeben", "Geben Sie die Hausnummer ein:", 1, 1, 10000, 1, &ok);
        if (!ok) return;

        int postleitzahl = QInputDialog::getInt(this, "Postleitzahl eingeben", "Geben Sie die Postleitzahl ein:", 10000, 10000, 99999, 1, &ok);
        if (!ok) return;

        QString gemeinde = QInputDialog::getText(this, "Gemeinde eingeben", "Geben Sie die Gemeinde ein:", QLineEdit::Normal, "", &ok);
        if (!ok) return;
        //QString gemeinde = "hochschule";
        // Adresse erstellen und speichern
        string nameString=name.toStdString();
        string strassString=strasse.toStdString();
        string gemeindeString=gemeinde.toStdString();

        Adresse* PAdresse=new Adresse(laengengrad,breitengrad,'a',strassString,hausnummer,postleitzahl,gemeindeString,nameString);
        MeinOrt.push_back(PAdresse);

        if(MeinOrt.size() > 2) { // Mehr als myPosition und der aktuelle Ort
            Ort* nearest = findNearestNeighbor(PAdresse);
            if(nearest) {
                PAdresse->setNachbar(nearest);
                nearest->setNachbar(PAdresse);
            }
        }
        AddRadioButton(nameString,breitengrad,laengengrad);
        drawConnections();
        QMessageBox::information(this, "Erfolg", "Die Adresse wurde erfolgreich angelegt!");
    }
    // POI erstellen
    else if (typ == "Point of Interest") {
        QString bemerkung = QInputDialog::getText(this, "Bemerkung eingeben", "Geben Sie eine Bemerkung ein:", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        QString kategorie = QInputDialog::getText(this, "Kategorie eingeben", "Geben Sie die Kategorie ein (z.B. Restaurant, Tankstelle):", QLineEdit::Normal, "", &ok);
        if (!ok) return;

        // POI erstellen und speichern
        string bemerkungString=bemerkung.toStdString();
        string kategorieString=kategorie.toStdString();
        string nameString=name.toStdString();

        PoI* pPoi=new PoI(laengengrad,breitengrad,'p',kategorieString,bemerkungString,nameString);
        MeinOrt.push_back(pPoi);

        if(MeinOrt.size() > 2) {
            Ort* nearest = findNearestNeighbor(pPoi);
            if(nearest) {
                pPoi->setNachbar(nearest);
                nearest->setNachbar(pPoi);
            }
        }


        AddRadioButton(nameString,breitengrad,laengengrad);
        drawConnections();

        QMessageBox::information(this, "Erfolg", "Der Point of Interest wurde erfolgreich angelegt!");
    }
}


void MainWindow::on_pushButton_alle_orten_clicked()
{


    if(MeinOrt.size()==0)
    {
        MeinOrt.push_back(myPosition);
    }
    QDialog dialog(this);
    dialog.setWindowTitle("Navigationssystem - Orte anzeigen");
    dialog.resize(1200, 600);

    // QTableWidget erstellen
    QTableWidget* table = new QTableWidget(&dialog);
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID", "Typ", "Name", "Breitengrad", "Längengrad", "Parameter", "Nachbarn"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int row = 0;
    if(MeinOrt.size()==0)
    {
        QMessageBox::information(this,"ERROR","Kein Element im Vector");
    }
    else
    {
        for(auto ort : MeinOrt)
        {
            if(!ort) continue;

            // Nachbarn Information erstellen
            const auto& nachbarn = ort->getNachbarn();

            // Erstelle eine Zeile für jeden Nachbarn
            int subRow = 0;
            do {
                table->insertRow(row);
                QString typ;
                QString parameters;
                QString nachbarnInfo = "";

                // Nur beim ersten Eintrag die Hauptinformationen anzeigen
                if(subRow == 0) {
                    if(PoI* poi = dynamic_cast<PoI*>(ort))
                    {
                        typ = "PoI";
                        parameters = QString::fromStdString(poi->getKategorie()) + ", " +
                                     QString::fromStdString(poi->getBemerkung());
                    }
                    else if(Adresse* adresse = dynamic_cast<Adresse*>(ort))
                    {
                        typ = "Adr";
                        parameters = QString::fromStdString(adresse->getStrasse()) + ", " +
                                     QString::number(adresse->getHausNummer()) + ", " +
                                     QString::number(adresse->getPLZ()) + ", " +
                                     QString::fromStdString(adresse->getGemeinde());
                    }
                    else
                    {
                        typ = "Unbekannt";
                        parameters = "Keine weiteren Parameter";
                    }

                    table->setItem(row, 0, new QTableWidgetItem(QString::number(ort->getID())));
                    table->setItem(row, 1, new QTableWidgetItem(typ));
                    table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(ort->getName())));
                    table->setItem(row, 3, new QTableWidgetItem(QString::number(ort->getLaengengrad())));
                    table->setItem(row, 4, new QTableWidgetItem(QString::number(ort->getBreitengrad())));
                    table->setItem(row, 5, new QTableWidgetItem(parameters));
                }

                // Nachbar Information für diese Zeile
                if(!nachbarn.empty() && subRow < nachbarn.size()) {
                    auto nachbar = nachbarn[subRow];
                    double distance = Rechnung(*ort, *nachbar);
                    nachbarnInfo = QString("%1 (ID: %2, Entfernung: %3 km)")
                                       .arg(QString::fromStdString(nachbar->getName()))
                                       .arg(nachbar->getID())
                                       .arg(distance, 0, 'f', 2);
                } else if(subRow == 0) {
                    nachbarnInfo = "Keine Nachbarn";
                }

                if(!nachbarnInfo.isEmpty()) {
                    table->setItem(row, 6, new QTableWidgetItem(nachbarnInfo));
                }

                row++;
                subRow++;
            } while(subRow < nachbarn.size()); // Wiederhole für jeden Nachbarn

            // Füge eine leere Zeile zwischen den Orten ein
            if(!nachbarn.empty()) {
                table->insertRow(row);
                row++;
            }
        }

        // Passe die Zeilenhöhen an
        for(int i = 0; i < table->rowCount(); i++) {
            table->resizeRowToContents(i);
        }

        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        layout->addWidget(table);

        dialog.setLayout(layout);
        dialog.exec();
    }
}


void MainWindow::on_action_2_Laden_triggered()
{
    string nummerString,laengenString,BreitenString,HausnummerString,PLZString;
    string Strasse,Kategorie,Bemerkung,zeile,Gemeinde,Name;

    int Newnum=0;
    int num=0;
    int plz=0;
    int Hausnummer=0;
    double Breiten,Laengen;
    ifstream DateiIn("C:/Users/moham/OneDrive/Documents/Navi_P4/Ort.txt");
    if(!DateiIn)
    {

        exit(-1);
    }
    for(auto m:MeinOrt)
        delete m;
    MeinOrt.clear();

    if(MeinOrt.size()==0)
    {
        MeinOrt.push_back(myPosition);
    }
    std::map<int, std::vector<int>> connections;

    getline(DateiIn,zeile);

    while(getline(DateiIn,zeile))
    {
        if(zeile=="PoI")
            break;

        stringstream tmp(zeile);
        getline(tmp,nummerString,';');
        getline(tmp,laengenString,';');
        getline(tmp,BreitenString,';');
        getline(tmp,Strasse,';');
        getline(tmp,HausnummerString,';');
        getline(tmp,PLZString,';');
        getline(tmp,Gemeinde,';');
        getline(tmp,Name,'\n');

        stringstream SNummer(nummerString);
        stringstream SLaenge(laengenString);
        stringstream SBreiten(BreitenString);
        stringstream SHausnummer(HausnummerString);
        stringstream SPlz(PLZString);


        SNummer>>num;
        SLaenge>>Laengen;
        SBreiten>>Breiten;
        SHausnummer>>Hausnummer;
        SPlz>>plz;

        if(num!=0)
        {
            Adresse* adresse= new Adresse(Laengen,Breiten,'a',num,Strasse,Hausnummer,plz,Gemeinde,Name);

            MeinOrt.push_back(adresse);



            if(MeinOrt.size() > 2) { // Mehr als myPosition und der aktuelle Ort
                Ort* nearest = findNearestNeighbor(adresse);
                if(nearest) {
                    adresse->setNachbar(nearest);
                    nearest->setNachbar(adresse);
                }
            }
            AddRadioButton(Name,Breiten,Laengen);



        }


        if(num>Newnum)
            Newnum=num;
    }




    while(getline(DateiIn,zeile))
    {
        if(zeile==" ")
            break;

        stringstream tmp(zeile);
        getline(tmp,nummerString,';');
        getline(tmp,laengenString,';');
        getline(tmp,BreitenString,';');
        getline(tmp,Kategorie,';');
        getline(tmp,Bemerkung,';');
        getline(tmp,Name,'\n');

        stringstream SNummer(nummerString);
        stringstream SLaengen(laengenString);
        stringstream SBreiten(BreitenString);

        SNummer>>num;
        SLaengen>>Laengen;
        SBreiten>>Breiten;

        if(num!=0)
        {

            PoI* Ppoi= new PoI(Laengen,Breiten,'p',num,Kategorie,Bemerkung,Name);
            MeinOrt.push_back(Ppoi);

            if(MeinOrt.size() > 2) {
                Ort* nearest = findNearestNeighbor(Ppoi);
                if(nearest) {
                    Ppoi->setNachbar(nearest);
                    nearest->setNachbar(Ppoi);
                }
            }
            AddRadioButton(Name,Breiten,Laengen);
        }

        if(num>Newnum)
            Newnum=num;
    }

    // Verbindungen laden
    while(getline(DateiIn,zeile) && !zeile.empty()) {
        stringstream tmp(zeile);
        string idStr, nachbarnStr;
        getline(tmp, idStr, ';');
        getline(tmp, nachbarnStr);

        if(!idStr.empty()) {
            int id = std::stoi(idStr);
            vector<int> nachbarIds;

            stringstream nachbarnStream(nachbarnStr);
            string nachbarId;
            while(getline(nachbarnStream, nachbarId, ',')) {
                if(!nachbarId.empty()) {
                    nachbarIds.push_back(std::stoi(nachbarId));
                }
            }
            connections[id] = nachbarIds;
        }
    }
    // Verbindungen herstellen
    for(const auto& [id, nachbarIds] : connections) {
        Ort* ort = nullptr;
        for(auto& o : MeinOrt) {
            if(o->getID() == id) {
                ort = o;
                break;
            }
        }

        if(ort) {
            for(int nachbarId : nachbarIds) {
                for(auto& o : MeinOrt) {
                    if(o->getID() == nachbarId) {
                        ort->setNachbar(o);
                        break;
                    }
                }
            }
        }
    }

    DateiIn.close();
    QMessageBox::information(this,"Laden","Ort erfolgreich geladen");

    Ort::setNummer(Newnum);
     drawConnections();
}


void MainWindow::on_action_3_Speichern_triggered()
{
    vector<Ort*> MeinAdresse;
    vector<Ort*>MeinPoI;


    for(auto m:MeinOrt)
    {
        if(m->getType()=='a')
            MeinAdresse.push_back(m);
        else
            MeinPoI.push_back(m);
    }

    ofstream DateiOut;
    DateiOut.open("C:/Users/moham/OneDrive/Documents/Navi_P4/Ort.txt");
    if(!DateiOut)
    {

        exit(-1);
    }
     //DateiOut << "Radiobutton Status: " << radiobuttonStatus.toStdString() << '\n';
    DateiOut<<"Adresse" <<'\n';

    for(auto A:MeinAdresse)
    {
        Adresse* PAdresse=dynamic_cast<Adresse*>(A);
        if(PAdresse)
        {
            DateiOut<<PAdresse->getID()<<';'
                     <<PAdresse->getLaengengrad()<<';'
                     <<PAdresse->getBreitengrad()<<';'
                     <<PAdresse->getStrasse()<<';'
                     <<PAdresse->getHausNummer()<<';'
                     <<PAdresse->getPLZ()<<';'
                     <<PAdresse->getGemeinde()<<';'
                    <<PAdresse->getName()<<endl;
        }
    }
    DateiOut<<"PoI"<<'\n';

    for(auto P:MeinPoI)
    {
        PoI* PPoi=dynamic_cast<PoI*>(P);
        if(PPoi)
        {
            DateiOut<<PPoi->getID()<<';'
                     <<PPoi->getLaengengrad()<<';'
                     <<PPoi->getBreitengrad()<<';'
                     <<PPoi->getKategorie()<<';'
                     <<PPoi->getBemerkung()<<';'
                     <<PPoi->getName()<<endl;


        }
    }

    // Speichere Nachbarschaftsbeziehungen



    DateiOut.close();

    QMessageBox::information(this,"Speichern","Orte erfolgreich gespeichert");
}




void MainWindow::on_pushButton_Entfernung_clicked()
{

    // QList<QRadioButton*> selectButton;
    // for(auto& button : MeinButton) {
    //     if(button->isChecked()) {
    //         selectButton.append(button);
    //     }
    // }
    // if (selectButton.size() != 2) {
    //     QMessageBox::warning(this, "Fehler", "Bitte wählen Sie genau zwei Orte aus.");
    //     return;
    // }
    // QString Name1 = selectButton[0]->text();
    // QString Name2 = selectButton[1]->text();
    // string NameString1 = Name1.toStdString();
    // string NameString2 = Name2.toStdString();

    // int idx1 = FindOrt(NameString1);
    // int idx2 = FindOrt(NameString2);

    // if(idx1 == -1 || idx2 == -1) {
    //     QMessageBox::warning(this, "Fehler", "Ein oder beide Orte wurden nicht gefunden.");
    //     return;
    // }
    // // Find and display route
    // findRoute(MeinOrt[idx1], MeinOrt[idx2]);
    QList<QRadioButton*> selectButton;
    for(auto& button : MeinButton) {
        if(button->isChecked()) {
            selectButton.append(button);
        }
    }
    if (selectButton.size() != 2) {
        QMessageBox::warning(this, "Fehler", "Bitte wählen Sie genau zwei Orte aus.");
        return;
    }
    QString Name1 = selectButton[0]->text();
    QString Name2 = selectButton[1]->text();
    string NameString1 = Name1.toStdString();
    string NameString2 = Name2.toStdString();
    int idx1 = FindOrt(NameString1);
    int idx2 = FindOrt(NameString2);
    if(idx1 == -1 || idx2 == -1) {
        QMessageBox::warning(this, "Fehler", "Ein oder beide Orte wurden nicht gefunden.");
        return;
    }
    double distance = Rechnung(*MeinOrt[idx1], *MeinOrt[idx2]);
    QString message = QString("Direkte Entfernung zwischen %1 und %2: %3 km")
                          .arg(Name1)
                          .arg(Name2)
                          .arg(distance, 0, 'f', 2);

    QMessageBox::information(this, "Entfernung", message);



}


void MainWindow::on_pushButton_information_clicked()
{
    QDialog dialog(this);  // Lokaler Dialog
    dialog.setWindowTitle("Navigationssystem - Orte anzeigen");
    dialog.resize(1200, 600);

    // QTableWidget erstellen
    QTableWidget* table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID", "Typ", "Name", "Breitengrad", "Laengengrad"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QList<QRadioButton*> selectButton;

    for(auto& button:MeinButton)
    {
        if(button->isChecked())
        {
            selectButton.append(button);
        }
    }

    if (selectButton.size() >1 ||selectButton.size()==0) {
        QMessageBox::warning(this, "Fehler", "Bitte wählen Sie genau einen Ort aus.");
        return;
    }

    QString Name1=selectButton[0]->text();

    string NameString1=Name1.toStdString();
    if(FindOrt(NameString1)==-1)
    {
       QMessageBox::information(this,"ERROR","Ort nicht gefunden");
    }
    else
    {
     Information(MeinOrt[FindOrt(NameString1)]);
    }
    }


 void MainWindow::on_pushButton_Mein_Ort_clicked()
    {
     if(MeinOrt.size()==0)
     {
         MeinOrt.push_back(myPosition);
     }
      showPosition(MeinOrt[0]->getBreitengrad(),MeinOrt[0]->getLaengengrad());
    }


    void MainWindow::on_pushButton_Verschieben_clicked()
    {
        bool ok; // speichert, ob der Benutzer eine gültige Auswahl getroffen hat.
        QStringList ortstypen = {"New Ort", "Existirende Ort"};
        QString typ = QInputDialog::getItem(this, "Ortstyp wählen", "Wählen Sie den Typ des Ortes:", ortstypen, 0, false, &ok);
        if (!ok) return;  // Abbruch
        if(typ=="Existirende Ort")
        {
            SchiebenExistierende();
            showPosition(MeinOrt[0]->getBreitengrad(),MeinOrt[0]->getLaengengrad());
        }
        if(typ=="New Ort")
        {
            SchiebenNewOrt();
            showPosition(MeinOrt[0]->getBreitengrad(),MeinOrt[0]->getLaengengrad());
        }
    }

    Ort *MainWindow::findNearestNeighbor(Ort *newOrt)
    {
        return findNearestNeighbor(MeinOrt, newOrt);
    }


    void MainWindow::on_pushButton_ortLoschen_clicked()
    {
        QList<QRadioButton*> selectButton;
        for(auto& button : MeinButton) {
            if(button->isChecked()) {
                selectButton.append(button);
            }
        }
        if(selectButton.size() != 1) {
            QMessageBox::warning(this, "Fehler", "Bitte wählen Sie genau einen Ort zum Löschen aus.");
            return;
        }
        QString name = selectButton[0]->text();
        string nameStr = name.toStdString();
        int idx = FindOrt(nameStr);

        if(idx != -1) {
            Ort* ortToDelete = MeinOrt[idx];
            if(ortToDelete == myPosition) {
                QMessageBox::warning(this, "Fehler", "Der aktuelle Standort kann nicht gelöscht werden.");
                return;
            }

            int result = QMessageBox::question(this, "Ort löschen",
                                               "Möchten Sie den Ort '" + name + "' wirklich löschen?",
                                               QMessageBox::Yes | QMessageBox::No);

            if(result == QMessageBox::Yes) {
                deleteOrt(ortToDelete);
                QMessageBox::information(this, "Erfolg", "Der Ort wurde erfolgreich gelöscht.");
            }
        }

    }


    void MainWindow::on_pushButton_routing_clicked()
    {
        QList<QRadioButton*> selectButton;
        for(auto& button : MeinButton) {
            if(button->isChecked()) {
                selectButton.append(button);
            }
        }
        if (selectButton.size() != 2) {
            QMessageBox::warning(this, "Fehler", "Bitte wählen Sie genau zwei Orte für das Routing aus.");
            return;
        }
        QString Name1 = selectButton[0]->text();
        QString Name2 = selectButton[1]->text();
        string NameString1 = Name1.toStdString();
        string NameString2 = Name2.toStdString();
        int idx1 = FindOrt(NameString1);
        int idx2 = FindOrt(NameString2);
        if(idx1 == -1 || idx2 == -1) {
            QMessageBox::warning(this, "Fehler", "Ein oder beide Orte wurden nicht gefunden.");
            return;
        }
        // Find and display route
        findRoute(MeinOrt[idx1], MeinOrt[idx2]);
    }

