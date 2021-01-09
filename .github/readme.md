# DriftTruck

## A game by Brandon Slaght

A weird alien ship swung over a few forests and abducted a bunch of trees for their experiemnts, but a malfunction caused them to all fall out of the ship.  As the owner of a terrible parking garage, you must push all the trees off the top of the garage so they don't interrupt business while avoiding being flattened yourself!

### Features:

#### Physics:
Truck will move left and right at a constant rate, as trucks do.  However, since the ground is very slippery it is also drifting all the time as is moves forward and backwards and is affectrd by drag.  There is also gravity pulling everything down

#### Plane-projected shadows:
These don't come from the light sources, because it is very important that the player can see them directly underneath the falling trees so that they can avoid them.  So they are just scaled to the trees height.

#### Collision detection:
Instead of a radius, I use a square, but it can still detect collisions in this way.  Special care is made to exclude trees that have already landed from ending the game.

#### Physical collision response (sort of):
I am not taking the collision normal but simply trading the velocity of the truck to the tree.  I also add more velocity, because maybe the truck is a little bouncy, but mostly because if I don't the tree will get stuck in the truck.

#### Dissapearing objects:
You can make objects dissapear from the scene by making thme move off the side of the garage (including the truck!)  This also deletes them from the objects vector.

#### Billboard:
As you can see, the pavement has grass.  However, this grass is only aligned to the camera in one direction because it does not look good to have blobs of grass directly facing the camera, as they should be at the same angle if they are on the same plane.  The camera doesn't move anyways (on purpose)

#### Particle system:
Any time an object lands, it creates a big noise and releases a lot of energy, thus dust is kicked up in random directions by them. These billboards don't fully face the camera either, because when they dissapear below the plane it looks very bad if the whole thing dissapears at once.  A small tilt means it dissapears more gradually.  Also, the billboards are deleted once the fall underground for efficiency.

#### Other notes:
The game quits when you lose  
The game is fullscreen  
The game can be quit with the esc key
