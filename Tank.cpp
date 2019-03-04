class Tank{
    public:
    double x = 0;
    double y = 0;
    int rotationAngle = 0;
    int rotationAngleMul = 2;
    int speedMultiplier = 2;
    bool local = true;
    bool inGame = true;
    int rsquared = 2304;
    int playerID = -1;

    Tank(int x, int y){
    }

    Tank(){
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

    void removeFromGame(){
        inGame = false;
    }

    void addBackIntoGame(){
        inGame = true;
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
