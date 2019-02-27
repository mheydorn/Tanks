class Tank{
    public:
    double x = 0;
    double y = 0;
    int rotationAngle = 0;
    int rotationAngleMul = 2;
    int speedMultiplier = 2;
    bool local = true;

    Tank(int x, int y){
    }

    Tank(double startx, double starty){
        x = startx;
        y = starty;
    }

    Tank(double startx, double starty, bool local){
        x = startx;
        y = starty;
        this->local = local;
    }


    void rotateLeft(){
        rotationAngle -= rotationAngleMul;
        rotationAngle = rotationAngle % 360;
        cout << "angle = " << rotationAngle << "\n";
    }
    void rotateRight(){
        rotationAngle += rotationAngleMul;
        rotationAngle = rotationAngle % 360;
        cout << "angle = " << rotationAngle << "\n";
    }

    void moveForward(){
        x += -(speedMultiplier * cos(degreeToRad(rotationAngle) + 3.14159/2));
        y += -(speedMultiplier * sin(degreeToRad(rotationAngle) + 3.14159/2));

        cout << "xPortion = " << -(speedMultiplier * cos(degreeToRad(rotationAngle) + 3.14159/2));
        cout << "yPortion = " << -(speedMultiplier * sin(degreeToRad(rotationAngle) + 3.14159/2));

        cout << "tankX = " << x << "\n";
        cout << "tankY = " << y << "\n";
    }
    void moveBackward(){
        x += (speedMultiplier * cos(degreeToRad(rotationAngle) + 3.14159/2));
        y += (speedMultiplier * sin(degreeToRad(rotationAngle) + 3.14159/2));

    }
};