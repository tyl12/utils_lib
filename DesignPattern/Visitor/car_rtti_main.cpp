#include <iostream>
#include <vector>

using namespace std;

class Wheel;
class Engine;
class Body;
class CarElement;

class CarElementVisitor{
    public:
        CarElementVisitor(){}
        virtual ~CarElementVisitor(){}
        //virtual void visit(CarElement* element)=0;
        virtual void visit(CarElement* element) = 0;
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
        virtual void visit(CarElement *element);
};
void CarElementPrintVisitor::visit(CarElement *element) {
    if (Wheel* ptr = dynamic_cast<Wheel*>(element)){
        cout << "Visiting " << ptr->name() << " wheel" << endl;
    }
    else if (Engine* ptr = dynamic_cast<Engine*>(element)){
        cout << "Visiting engine" << endl;
    }
    else if (Body* ptr = dynamic_cast<Body*>(element)){
        cout << "Visiting body" << endl;
    }
}

class CarElementDoVisitor: public CarElementVisitor{
    public:
        CarElementDoVisitor(){}
        virtual ~CarElementDoVisitor(){}
        virtual void visit(CarElement *element);
};

void CarElementDoVisitor::visit(CarElement *element) {
    if (Wheel* ptr = dynamic_cast<Wheel*>(element)){
        cout << "Kicking my " << ptr->name() << " wheel" << endl;
    }
    else if (Engine* ptr = dynamic_cast<Engine*>(element)){
        cout << "Starting my engine" << endl;
    }
    else if (Body* ptr = dynamic_cast<Body*>(element)){
        cout << "Moving my body" << endl;
    }
}

int main(){
    CarElementVisitor* visitor =  new CarElementDoVisitor;
    Car car;
    car.visit_car(visitor);
    return 0;

}
