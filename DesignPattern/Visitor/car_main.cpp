#include <iostream>
#include <vector>

using namespace std;

class Wheel;
class Engine;
class Body;
class CarElementVisitor{
    public:
        CarElementVisitor(){}
        virtual ~CarElementVisitor(){}
        //virtual void visit(CarElement* element)=0;
        virtual void visit(Wheel *wheel) = 0;
        virtual void visit(Engine *engine) = 0;
        virtual void visit(Body *body) = 0;
};


class CarElement {
    public:
        virtual ~CarElement() {}
        virtual void accept(CarElementVisitor *visitor) = 0;
};

class Wheel : public CarElement {
    public:
        explicit Wheel(string name):name_(name){};
        virtual ~Wheel(){};
        virtual void accept(CarElementVisitor *visitor){
            visitor->visit(this);
        }
        string name() const {return name_;}
    private:
        string name_;
};

class Engine : public CarElement {
    public:
        virtual ~Engine(){};
        virtual void accept(CarElementVisitor *visitor){
            visitor->visit(this);
        }
};

class Body : public CarElement {
    public:
        virtual ~Body(){};
        virtual void accept(CarElementVisitor *visitor){
            visitor->visit(this);
        }
};

class Car {
    public:
        Car();
        virtual ~Car();
        void visit_car(CarElementVisitor *visitor);
    private:
        void visit_elements(CarElementVisitor *visitor);
        vector<CarElement *> elements_array_;
};

Car::Car() {
    elements_array_.push_back(new Wheel("front left"));
    elements_array_.push_back(new Wheel("front right"));
    elements_array_.push_back(new Body());
    elements_array_.push_back(new Engine());
}

Car::~Car() {
    for(unsigned int i = 0; i < elements_array_.size(); ++i) {
        CarElement *element = elements_array_[i];
        delete element;
    }
}

void Car::visit_car(CarElementVisitor *visitor) {
    visit_elements(visitor);
}

void Car::visit_elements(CarElementVisitor *visitor) {
    for(unsigned int i = 0; i < elements_array_.size(); ++i) {
        CarElement *element = elements_array_[i];
        element->accept(visitor);
    }
}

class CarElementPrintVisitor: public CarElementVisitor{
    public:
        CarElementPrintVisitor(){}
        virtual ~CarElementPrintVisitor(){}
        virtual void visit(Wheel *wheel);
        virtual void visit(Engine *engine);
        virtual void visit(Body *body);
};
void CarElementPrintVisitor::visit(Wheel *wheel) {
    cout << "Visiting " << wheel->name() << " wheel" << endl;
}

void CarElementPrintVisitor::visit(Engine *engine) {
    cout << "Visiting engine" << endl;
}

void CarElementPrintVisitor::visit(Body *body) {
    cout << "Visiting body" << endl;
}

class CarElementDoVisitor: public CarElementVisitor{
    public:
        CarElementDoVisitor(){}
        virtual ~CarElementDoVisitor(){}
        virtual void visit(Wheel *wheel);
        virtual void visit(Engine *engine);
        virtual void visit(Body *body);
};

void CarElementDoVisitor::visit(Wheel *wheel) {
    cout << "Kicking my " << wheel->name() << " wheel" << endl;
}

void CarElementDoVisitor::visit(Engine *engine) {
    cout << "Starting my engine" << endl;
}

void CarElementDoVisitor::visit(Body *body) {
    cout << "Moving my body" << endl;
}

int main(){
    CarElementVisitor* visitor =  new CarElementDoVisitor;
    Car car;
    car.visit_car(visitor);
    return 0;

}
