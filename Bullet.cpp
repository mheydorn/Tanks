class Bullet{
    public:
    double dx;
    double dy;
    double x;
    double y;
    int angle;
    Bullet(int tankIndexThatFired){
        this->angle = tanks[tankIndexThatFired]->rotationAngle;
        dx = -(bulletSpeedMultiplier * cos(degreeToRad(angle) + 3.14159/2));
        dy = -(bulletSpeedMultiplier * sin(degreeToRad(angle) + 3.14159/2));

        this->x = tanks[tankIndexThatFired]->x;
        this->y = tanks[tankIndexThatFired]->y;
    
    }
    void update(){
        x += dx;
        y += dy;
    }
};
