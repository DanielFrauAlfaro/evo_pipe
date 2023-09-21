------- Contenidos -------
  Hay diferentes carpetas relacionadas cada una con un experimento diferente.
  Dentro de cada una hay:
    - 0_color.png: imagen RGB del objeto
    - 0_info.txt: información sobre los puntos calculados y vectores de posicionamiento del robot
    - 0_object.pcd: nube de puntos del objeto
    - 0_PC.pcd: nube capturada
    - 0_PC_graspPoints.pcd: nube capturada con los puntos de agarre en colores
    - 0_plane.pcd: plano sobre el que se encuentra el objeto
    - <nombre_objeto>_errores.txt: descripción de los errores que se detectan en el experimento
    - <nombre_objeto>_puntos.png: imagen de los puntos en RVIZ
    
  El que se le pasaría a geoGrasp sería:
    - 0_plane.pcd
    - 0_object.pcd


------- Códigos de color de los puntos -------
  - Rojo: pulgar 
  - Verde y azul: dedos enfrentados
  
  Deberían aparecer:
    - El punto rojo enfrentado a los otros dos, en línea con su punto medio. Así:
    
    		*         *
    		
    		
    		     * 
    
    
------- Parámetros -------
  tip_size = 25
  apertures = {{-0.036,-0.036},{0.036,0.036}}
  number_fingers = 3
  unique_mobility = 0
  grasps_track = 5
  
  
  

