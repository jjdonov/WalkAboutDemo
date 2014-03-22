struct particle
{
    float lifetime;                       // total lifetime of the particle
    float decay;                          // decay speed of the particle
    float xpos,ypos,zpos;                 // position of the particle
    float xspeed,yspeed,zspeed;           // speed of the particle
    bool active;                          // is particle active or not?
    
    particle()
    {
        lifetime = rand()%100; //change to random
        decay = float(rand() % 5); // change to random
        xpos = 15.0 + float(rand()%15);
        zpos = -32.0 - float(rand()%4);
        ypos = 0.0;
        xspeed = sin((0.0f*PI)/180.0);
        //xspeed = sin((fire*PI)/180.0); fire is from main and is incremened on evolvePartilce()
        yspeed = float((rand()%100 + 1) / 500.0);//250.0);
        zspeed = 0.0;
        active = true;
    }
};