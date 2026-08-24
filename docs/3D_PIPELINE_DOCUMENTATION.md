# Guía de Pipeline 3D: Creación de Assets, Texturizado, Rigging y Animaciones (Blender a Unreal Engine 5.8)

Bienvenido a la guía técnica y de flujo de trabajo de arte 3D para **ProjectF (Good Service)** en Unreal Engine 5.8.

Este manual documenta de forma completa, estructurada y paso a paso el pipeline integral de producción de activos tridimensionales: desde el modelado y desenrollado UV en Blender, texturizado PBR en Adobe Substance 3D Painter, corrección y optimización de colisiones y materiales translúcidos/emisivos en Unreal Engine 5.8, hasta la captura de movimiento sin marcadores mediante MetaHuman Animator, retargeting con Rokoko y asignación de pesos (*weight painting*).

---

## Tabla de Contenidos

1. [Creación, Aplicación y Exportación de Texturas (Blender $\rightarrow$ Substance Painter $\rightarrow$ Unreal Engine)](#1-creación-aplicación-y-exportación-de-texturas)
   - [1.1 Preparar el Modelo en Blender y Mapa UV](#11-preparar-el-modelo-en-blender)
   - [1.2 Exportar el Modelo desde Blender (FBX)](#12-exportar-el-modelo-desde-blender)
   - [1.3 Importar el Modelo en el Programa de Texturizado](#13-importar-el-modelo-en-el-programa-de-texturizado)
   - [1.4 Aplicar Materiales y Texturas al Modelo](#14-aplicar-materiales-y-texturas-al-modelo)
   - [1.5 Exportar las Texturas PBR](#15-exportar-las-texturas)
   - [1.6 Volver a Abrir el Modelo en Blender](#16-volver-a-abrir-el-modelo-en-blender)
   - [1.7 Activar el Add-on Node Wrangler](#17-activar-node-wrangler)
   - [1.8 Abrir el Espacio de Trabajo Shading](#18-abrir-el-espacio-de-trabajo-shading)
   - [1.9 Crear un Material](#19-crear-un-material)
   - [1.10 Importar Todas las Texturas Automáticamente (Ctrl + Shift + T)](#110-importar-todas-las-texturas-automáticamente)
   - [1.11 Comprobar que el Material se Vea Correctamente](#111-comprobar-que-el-material-se-vea-correctamente)
   - [1.12 Guardar el Proyecto de Blender](#112-guardar-el-proyecto-de-blender)
   - [1.13 Preparar el Modelo para Unreal Engine](#113-preparar-el-modelo-para-unreal-engine)
   - [1.14 Exportar el Modelo Final desde Blender](#114-exportar-el-modelo-desde-blender)
   - [1.15 Importar el Modelo en Unreal Engine](#115-importar-el-modelo-en-unreal-engine)
   - [1.16 Importar las Texturas en Unreal Engine](#116-importar-las-texturas-en-unreal-engine)
   - [1.17 Revisar el Material en Unreal Engine](#117-revisar-el-material-en-unreal-engine)
   - [1.18 Comprobar el Modelo dentro del Escenario](#118-comprobar-el-modelo-dentro-del-escenario)
2. [Solución de Problemas: Lectura Incorrecta de Texturas en Unreal](#2-problema-de-lectura-incorrecta-de-texturas-en-unreal)
   - [2.1 Abrir la Ventana de Exportación en Substance Painter](#21-abrir-la-ventana-de-exportación)
   - [2.2 Localizar la Plantilla de Exportación (Output Template)](#22-localizar-la-plantilla-de-exportación)
   - [2.3 Cambiar la Plantilla por una Compatible con Unreal Engine](#23-cambiar-la-plantilla-por-una-compatible-con-unreal-engine)
   - [2.4 Exportar Nuevamente las Texturas](#24-exportar-nuevamente-las-texturas)
   - [2.5 Reemplazar y Actualizar Texturas en Unreal Engine](#25-reemplazar-las-texturas-en-unreal-engine)
3. [Configuración y Corrección de Colisiones en Unreal Engine](#3-correción-de-colisiones-en-unreal)
   - [3.1 Abrir el Modelo en el Static Mesh Editor](#31-abrir-el-modelo-que-presenta-el-problema)
   - [3.2 Visualizar la Colisión Actual](#32-mostrar-la-colisión-actual)
   - [3.3 Eliminar Colisiones Incorrectas](#33-eliminar-una-colisión-incorrecta)
   - [3.4 Crear una Colisión Simple (Box / Sphere / Capsule)](#34-crear-una-colisión-sencilla)
   - [3.5 Generación con Auto Convex Collision](#35-utilizar-auto-convex-collision)
   - [3.6 Ajustar la Precisión de la Descomposición Convexa](#36-ajustar-la-precisión-de-la-colisión)
   - [3.7 Colisión Simple vs. Colisión Compleja](#37-colisión-simple-y-colisión-compleja)
   - [3.8 Guardar y Aplicar Cambios](#38-guardar-los-cambios)
   - [3.9 Validación de Colisión en Modo Play](#39-probar-la-colisión-dentro-del-juego)
4. [Creación de Materiales Especiales: Cristalinos y Luminosos](#4-creación-de-materiales-luminosos-y-cristalinos-en-unreal)
   - [A. Creación de un Material Cristalino (Translucent Glass)](#a-creación-de-un-material-cristalino)
     - [4.1.1 Crear un Nuevo Material](#411-crear-un-nuevo-material)
     - [4.1.2 Configurar Blend Mode en Translucent](#412-cambiar-el-material-para-permitir-transparencia)
     - [4.1.3 Configurar el Color Base](#413-configurar-el-color-del-cristal)
     - [4.1.4 Configurar la Opacidad](#414-configurar-la-transparencia)
     - [4.1.5 Ajustar Rugosidad (Roughness)](#415-ajustar-la-rugosidad-del-cristal)
     - [4.1.6 Compilación y Aplicación del Material](#416-revisar-el-material)
   - [B. Creación de un Material Luminoso (Emissive Color)](#b-creación-de-un-material-luminoso)
     - [4.2.1 Crear el Material Emisivo](#421-crear-el-material-luminoso)
     - [4.2.2 Crear el Parámetro de Color](#422-crear-el-color-de-la-luz)
     - [4.2.3 Parámetro de Multiplicación e Intensidad](#423-crear-un-control-para-la-intensidad)
     - [4.2.4 Calibración de Intensidad y Bloom](#424-ajustar-la-intensidad)
     - [4.2.5 Aplicación sobre el Modelo](#425-aplicar-el-material-al-objeto)
     - [4.2.6 Distinción Crítica: Material Emisivo vs. Luces Físicas](#426-importante-material-luminoso-no-siempre-significa-una-luz-real)
5. [Animación con MetaHuman Animator, Captura Sin Marcadores y Retargeting](#5-desarrollo-de-animaciones-con-metahuman-y-correción-de-huesos-para-animaciones)
   - [5.1 Grabación y Selección del Video de Referencia](#51-grabar-un-movimiento-o-conseguir-un-video)
   - [5.2 Habilitar Plugins de MetaHuman en Unreal Engine](#52-activar-las-herramientas-de-metahuman)
   - [5.3 Apertura de Capture Manager en Live Link Hub](#53-abrir-capture-manager)
   - [5.4 Configurar Ingesta Mono Video Ingest](#54-crear-un-mono-video-ingest)
   - [5.5 Importación y Procesamiento de Video](#55-importar-el-video)
   - [5.6 Creación del Asset MetaHuman Performance](#56-crear-un-metahuman-performance)
   - [5.7 Asignación de Datos de Captura (Capture Data)](#57-asignar-la-captura)
   - [5.8 Activación del Rastreo Corporal (Body Tracking)](#58-activar-body-tracking)
   - [5.9 Recorte de Rango en Línea de Tiempo](#59-seleccionar-el-fragmento-del-video)
   - [5.10 Procesamiento del Performance](#510-procesar-la-animación)
   - [5.11 Exportar Secuencia de Animación de MetaHuman](#511-exportar-la-animación-obtenida)
   - [5.12 Exportar Animación a Archivo FBX](#512-exportar-la-animación-como-fbx)
   - [5.13 Carga del Personaje Low-Poly en Blender](#513-abrir-blender)
   - [5.14 Importar la Animación FBX de MetaHuman en Blender](#514-importar-la-animación-de-metahuman)
   - [5.15 Configurar el Add-on Rokoko para Blender](#515-instalar-y-activar-rokoko)
   - [5.16 Selección de Armatures Source y Target](#516-abrir-la-herramienta-retargeting)
   - [5.17 Generación del Bone List Automático](#517-crear-el-bone-list)
   - [5.18 Mapeo y Ajuste Manual de Jerarquía de Huesos](#518-revisar-manualmente-los-huesos)
   - [5.19 Compatibilidad de Poses de Reposo (Rest Poses)](#519-comprobar-las-poses-de-ambos-esqueletos)
   - [5.20 Ejecución del Retargeting](#520-ejecutar-el-retargeting)
   - [5.21 Previsualización de la Animación en Blender](#521-reproducir-la-animación)
   - [5.22 Checklist de Exportación del Personaje](#522-preparar-el-personaje-para-exportar)
   - [5.23 Exportar Personaje Animado a FBX](#523-exportar-desde-blender-como-fbx)
   - [5.24 Importar Skeletal Mesh y Animaciones en Unreal](#524-importar-nuevamente-en-unreal-engine)
   - [5.25 Reutilización de Skeleton Existente](#525-si-el-personaje-ya-existe-en-unreal)
   - [5.26 Verificación de la Secuencia de Animación](#526-probar-la-animación-en-unreal)
   - [5.27 Integración en Sequencer, Animation Blueprints y Gameplay](#527-utilizarla-dentro-del-proyecto)
6. [Guía de Asignación y Calibración de Pesos (Weight Painting)](#6-guía-de-asignacion-de-pesos)
   - [6.1 Verificación del Esqueleto y Malla Base](#61-comprobar-que-el-personaje-tenga-un-esqueleto)
   - [6.2 Emparentado con Pesos Automáticos (With Automatic Weights)](#62-vincular-la-malla-al-esqueleto)
   - [6.3 Comprobación de Deformación en Pose Mode](#63-probar-la-deformación)
   - [6.4 Ingreso al Modo Weight Paint y Lectura de Gradientes](#64-entrar-en-weight-paint)
   - [6.5 Selección de Hueso Específico](#65-seleccionar-el-hueso-que-se-quiere-corregir)
   - [6.6 Pinceles: Añadir, Restar, Suavizar y Promediar](#66-añadir-o-quitar-influencia)
   - [6.7 Aislamiento y Corrección de Vértices Cruzados](#67-corregir-zonas-que-se-mueven-con-el-hueso-equivocado)
   - [6.8 Normalización y Limpieza de Influencias](#68-normalizar-los-pesos)
   - [6.9 Puntos Críticos de Articulación](#69-revisar-articulaciones-importantes)
   - [6.10 Validación Dinámica con Playback de Animación](#610-probar-los-pesos-con-la-animación)
   - [6.11 Ciclo de Iteración de Deformación](#611-repetir-las-correcciones-necesarias)
7. [Guía y Checklist de Limpieza de Modelos para Exportación](#7-como-limpiar-y-preparar-modelos-para-exportacion-a-unreal)
   - [7.1 Organización de la Escena y Nomenclatura](#71-revisar-y-organizar-el-modelo)
   - [7.2 Fusión de Vértices Duplicados (Merge By Distance)](#72-limpiar-la-geometría)
   - [7.3 Verificación y Recálculo de Normales](#73-revisar-las-normales)
   - [7.4 Detección de Geometría No Múltiple (Non-Manifold)](#74-revisar-errores-non-manifold)
   - [7.5 Aplicación de Escala y Rotación (Apply Transforms)](#75-aplicar-las-transformaciones)
   - [7.6 Dimensiones, Ubicación y Origen del Objeto (Pivot Z=0)](#76-revisar-tamaño-posición-y-origen)
   - [7.7 Validación del Desenvolvimiento UV](#77-revisar-el-mapa-uv)
   - [7.8 Depuración de Slots de Materiales](#78-revisar-los-materiales)
   - [7.9 Aplicación Definitiva de Modificadores](#79-revisar-los-modificadores)
   - [7.10 Unión de Mallas (Join Ctrl+J)](#710-unir-piezas-cuando-sea-necesario)
   - [7.11 Selección Exclusiva de Elementos Requeridos](#711-seleccionar-el-modelo-para-exportar)
   - [7.12 Configuración de Exportación FBX](#712-exportar-a-fbx)
   - [7.13 Importación y Checklist Final en Unreal Engine 5.8](#713-importar-y-comprobar-en-unreal-engine)

---

## 1. Creación, Aplicación y Exportación de Texturas

En esta sección se detalla el ciclo completo de vida de un asset 3D desde su modelado en Blender, su texturizado en Adobe Substance 3D Painter con materiales PBR (*Physically Based Rendering*), la asignación de nodos en Blender para previsualización y su importación definitiva a Unreal Engine 5.8.

### 1.1 Preparar el Modelo en Blender

Antes de pintar o colocar materiales, debemos asegurarnos de que el modelo esté preparado para recibir coordenadas de textura. Una **textura** es un mapa bidimensional de píxeles proyectado sobre la superficie poligonal para determinar color (*Base Color*), microdetalles y relieve (*Normal*), reflectividad (*Roughness*) y propiedades metálicas (*Metallic*).

#### ¿Qué es un Mapa UV?
El mapa UV representa el despliegue bidimensional de la superficie tridimensional del objeto sobre un plano cartesiano \((U, V)\). Si el modelo fuera una caja de cartón tridimensional, el mapa UV equivale a cortar y desplegar sus seis caras sobre una mesa de dibujo.

**Pasos en Blender:**
1. Abrir Blender.
2. Cargar el archivo de escena que contiene el modelo.
3. Seleccionar el objeto haciendo clic sobre él en el *Viewport* o en el *Outliner*.
4. Cambiar al espacio de trabajo **UV Editing** en la barra superior.
5. Comprobar que todas las caras posean islas UV proyectadas sin distorsiones extremas.
6. Verificar que las islas UV no se solapen (*overlapping*), a menos que se trate de simetría intencional.
7. Confirmar que el mapa UV ocupe eficientemente el espacio \(0 \rightarrow 1\).

![Espacio de trabajo UV Editing en Blender mostrando el mapa UV de un modelo](images/3d_pipeline/01_blender_uv_editing.jpg)

---

### 1.2 Exportar el Modelo desde Blender

Para transferir la geometría hacia la suite de texturizado (Adobe Substance 3D Painter), utilizamos el formato de intercambio estándar de la industria: **FBX**.

**Pasos de exportación:**
1. Seleccionar en el *Viewport* el modelo u objetos específicos que se van a texturizar.
2. Dirigirse al menú superior: `File > Export > FBX (.fbx)`.
3. Navegar hasta el directorio de destino del proyecto.
4. Asignar un nombre descriptivo bajo la nomenclatura del proyecto (ejemplo: `Mesa_Cafeteria.fbx`).
5. En el panel lateral derecho de opciones del exportador, activar la casilla **Selected Objects** para evitar exportar cámaras, luces o mallas auxiliares.
6. Hacer clic en **Export FBX**.

| Menú Exportar FBX | Diálogo y Opciones de Exportación FBX |
| :---: | :---: |
| ![Menú File Export FBX en Blender](images/3d_pipeline/02_blender_export_menu.jpg) | ![Configuración de exportación FBX en Blender](images/3d_pipeline/03_blender_export_fbx_dialog.png) |

---

### 1.3 Importar el Modelo en el Programa de Texturizado

Abrimos **Adobe Substance 3D Painter** para configurar el proyecto e importar la geometría FBX.

**Pasos:**
1. Iniciar Adobe Substance 3D Painter.
2. Ir a `File > New...` (o `Ctrl + N`).
3. En el campo **File**, presionar **Select...** y seleccionar el archivo `.fbx` exportado desde Blender.
4. Establecer la resolución del documento (ejemplo: `2048` o `1024`).
5. Confirmar que el formato de normales esté configurado según el motor (*DirectX* para Unreal Engine, o *OpenGL* si se planea convertir después).
6. Presionar **OK** y esperar a que el visor 3D/2D cargue la geometría y sus coordenadas UV.

![Ventana de creación de nuevo proyecto en Adobe Substance 3D Painter](images/3d_pipeline/04_substance_new_project.png)

> [!WARNING]
> Si la malla presenta caras oscuras, invertidas o partes estiradas al cargarse en Substance Painter, detén el proceso y regresa a Blender a revisar la orientación de normales (`Shift + N`) o el despliegue UV.

---

### 1.4 Aplicar Materiales y Texturas al Modelo

Con la geometría cargada, procedemos a generar las capas de material, máscaras inteligentes y texturas de superficie.

**Pasos de texturizado:**
1. Explorar el panel de recursos (**Assets / Shelf**) y seleccionar materiales base (madera, metal cepillado, plástico mate, tela, concreto).
2. Arrastrar el material o *Smart Material* deseado directamente sobre el modelo en el visor 3D o sobre la lista de capas (**Layers**).
3. Ajustar los parámetros del material en el panel **Properties - Fill**:
   - Tono y saturación de color (*Base Color*).
   - Rugosidad de superficie (*Roughness*).
   - Comportamiento metálico (*Metallic*).
   - Escala y orientación de patrones de textura (*UV Scale, Rotation*).
4. Si el modelo se compone de diferentes piezas o conjuntos de texturas (*Texture Sets*), aplicar y pintar los materiales respectivos en cada conjunto.
5. Añadir detalles de desgaste, suciedad o bordes resaltados utilizando generadores y máscaras de oclusión ambiental y curvatura.

![Panel de materiales y visor 3D en Adobe Substance 3D Painter](images/3d_pipeline/05_substance_materials_shelf.jpg)

---

### 1.5 Exportar las Texturas

Una vez concluido el texturizado, generamos los mapas de imagen independientes que controlan el shader PBR:

* **Base Color / Albedo:** Información cromática pura sin sombras fijas.
* **Normal Map:** Microrelieves y detalles superficiales que interactúan con la iluminación sin añadir polígonos.
* **Roughness:** Control de dispersión de luz (micro-rugosidad vs. reflectividad brillante).
* **Metallic:** Definición de conductores eléctricos (metales) vs. dieléctricos (no metales).

**Pasos de exportación:**
1. En Substance Painter, ir al menú `File > Export Textures...` (o `Ctrl + Shift + E`).
2. En la pestaña **Settings**, crear o definir una carpeta exclusiva para las texturas generadas (ejemplo: `Texturas_Mesa/`).
3. Seleccionar el directorio como ruta de salida (**Output directory**).
4. Seleccionar la plantilla de exportación adecuada (**Output template**).
5. Seleccionar la resolución de los mapas (ejemplo: `2048 x 2048`).
6. Presionar **Export**.

| Panel de Capas y Propiedades | Menú Export Textures |
| :---: | :---: |
| ![Capas y configuración de material en Substance Painter](images/3d_pipeline/06_substance_material_layers.jpg) | ![Menú de exportación de texturas](images/3d_pipeline/07_substance_export_textures_menu.jpg) |

| Ventana de Configuración de Exportación | Directorio de Texturas Exportadas |
| :---: | :---: |
| ![Ventana de Export Textures en Substance Painter](images/3d_pipeline/08_substance_export_textures_window.png) | ![Archivos de mapas generados en disco](images/3d_pipeline/09_textures_exported_folder.png) |

---

### 1.6 Volver a Abrir el Modelo en Blender

Regresamos a Blender para verificar la correcta asignación de los mapas sobre el shader Principled BSDF antes de pasar al motor.

---

### 1.7 Activar Node Wrangler

El add-on oficial **Node Wrangler** automatiza la conexión simultánea de todos los mapas PBR ahorrando tiempo de conexión manual.

**Pasos de activación:**
1. En Blender, ir al menú: `Edit > Preferences`.
2. Dirigirse a la sección **Add-ons**.
3. En el campo de búsqueda, escribir: `Node Wrangler`.
4. Marcar la casilla de activación a la izquierda del complemento.
5. Cerrar la ventana de preferencias (los cambios se guardan automáticamente).

![Búsqueda y activación del add-on Node Wrangler en Blender Preferences](images/3d_pipeline/10_blender_addon_node_wrangler.png)

---

### 1.8 Abrir el Espacio de Trabajo Shading

1. En la barra superior de Blender, hacer clic en la pestaña **Shading**.
2. En la mitad superior se presentará el visor 3D en modo de previsualización de materiales (*Material Preview*).
3. En la mitad inferior se presentará el editor de nodos (**Shader Editor**), donde interactúan los nodos `Principled BSDF` y `Material Output`.

![Espacio de trabajo Shading en Blender con vista 3D y Shader Editor](images/3d_pipeline/11_blender_shading_workspace.jpg)

---

### 1.9 Crear un Material

1. Seleccionar el objeto en el visor 3D.
2. Si el objeto no posee un material asignado, presionar el botón **New** en el encabezado del Shader Editor.
3. Se generará automáticamente el nodo principal `Principled BSDF` conectado al nodo `Material Output`.

![Ubicación del botón New Material en el editor de Shading](images/3d_pipeline/12_blender_new_material_button.jpg)

---

### 1.10 Importar Todas las Texturas Automáticamente

1. Hacer un clic sobre el nodo **Principled BSDF** para mantenerlo seleccionado (contorno resaltado).
2. Presionar el atajo de teclado: `Ctrl + Shift + T` (función provista por Node Wrangler).
3. Se abrirá la ventana de selección de archivos de Blender.
4. Navegar a la carpeta donde Substance Painter exportó los mapas (`BaseColor`, `Normal`, `Roughness`, `Metallic`).
5. Seleccionar todas las imágenes correspondientes al conjunto de material.
6. Presionar el botón **Principled Texture Setup**.
7. Node Wrangler analizará los sufijos de los archivos y conectará automáticamente:
   - `BaseColor` $\rightarrow$ `Base Color` (Espacio de color *sRGB*)
   - `Roughness` $\rightarrow$ `Roughness` (Espacio de color *Non-Color*)
   - `Metallic` $\rightarrow$ `Metallic` (Espacio de color *Non-Color*)
   - `Normal` $\rightarrow$ Nodo `Normal Map` $\rightarrow$ `Normal` (Espacio de color *Non-Color*)

![Ventana de selección de texturas tras presionar Ctrl + Shift + T](images/3d_pipeline/13_blender_principled_texture_setup.png)

---

### 1.11 Comprobar que el Material se Vea Correctamente

En el visor 3D, verificar el modelo bajo la vista **Material Preview**:
* Coincidencia cromática fiel con el diseño original.
* Ausencia de artefactos de estiramiento o costuras visibles (*seams*).
* Grado correcto de brillo y rugosidad en las caras pulidas vs. mate.
* Comportamiento adecuado del mapa de normales en las hendiduras y relieves.

![Modelo con texturas aplicadas visualizado en modo Material Preview en Blender](images/3d_pipeline/14_blender_material_preview_chair.jpg)

---

### 1.12 Guardar el Proyecto de Blender

Presionar `Ctrl + S` (`File > Save`) para almacenar la escena, conservando el setup de nodos y jerarquías de materiales.

---

### 1.13 Preparar el Modelo para Unreal Engine

Antes de la exportación final, realizar la verificación estándar:
* **Escala:** Factores \(X=1, Y=1, Z=1\).
* **Rotación:** Ángulos en \(0^\circ, 0^\circ, 0^\circ\).
* **Pivote / Origen:** Posicionado en la base inferior del objeto (\(Z=0\)) para un contacto perfecto con el suelo en Unreal.
* **Normales:** Todas orientadas uniformemente hacia el exterior.

---

### 1.14 Exportar el Modelo Final desde Blender

1. Seleccionar el objeto preparado.
2. Ir a `File > Export > FBX (.fbx)`.
3. Activar **Selected Objects**.
4. Definir nombre de exportación final (ejemplo: `Mesa_Cafeteria_Final.fbx`).
5. En la sección **Transform**, verificar: Forward = `-Y Forward`, Up = `Z Up`, Scale = `1.0`.
6. Presionar **Export FBX**.

![Opciones finales de exportación FBX en Blender](images/3d_pipeline/15_blender_export_fbx_final.png)

---

### 1.15 Importar el Modelo en Unreal Engine

1. Abrir el proyecto **ProjectF** en Unreal Engine 5.8.
2. Abrir el **Content Drawer** (`Ctrl + Space`).
3. Crear o navegar a la carpeta de destino del activo (ejemplo: `/Game/Art/Models/Furniture/`).
4. Presionar el botón **Import** o arrastrar el archivo `.fbx` directamente a la carpeta.
5. En la ventana **FBX Import Options**:
   - **Mesh > Skeletal Mesh:** Desactivado (para mallas estáticas).
   - **Mesh > Build Nanite:** Activado (para mallas de alta resolución) o desactivado según el requerimiento.
   - **Mesh > Generate Missing Collision:** Activado si se desea una colisión preliminar automática.
   - **Material > Search Location:** Local.
   - **Material > Material Import Method:** *Create New Materials*.
6. Presionar **Import All**.

| Explorador Content Drawer | Cuadro de Opciones FBX Import Options |
| :---: | :---: |
| ![Content Drawer en Unreal Engine](images/3d_pipeline/16_ue5_content_drawer.jpg) | ![Cuadro de opciones de importación FBX en UE5](images/3d_pipeline/18_ue5_fbx_import_options.png) |

---

### 1.16 Importar las Texturas en Unreal Engine

Si los mapas no fueron empaquetados dentro del FBX o se desea máxima fidelidad cromática:
1. En el Content Drawer, ingresar a la carpeta de texturas (ejemplo: `/Game/Art/Textures/`).
2. Presionar **Import** y seleccionar las imágenes `.png` o `.tga` exportadas de Substance.
3. Unreal Engine detectará e importará los mapas, asignando automáticamente la compresión `Normalmap (DXT5)` a los archivos de normales y `sRGB` al Base Color.

![Texturas importadas visualizadas en el Content Drawer de Unreal Engine](images/3d_pipeline/19_ue5_textures_imported.jpg)

---

### 1.17 Revisar el Material en Unreal Engine

1. Hacer doble clic sobre el material asignado al modelo importado para abrir el **Material Editor**.
2. Verificar que los nodos `Texture Sample` estén conectados a los pines del nodo principal:
   - Textura RGB Base Color $\rightarrow$ `Base Color`
   - Textura Normal Map $\rightarrow$ `Normal`
   - Textura Roughness $\rightarrow$ `Roughness`
   - Textura Metallic $\rightarrow$ `Metallic`
3. Si se utilizó un mapa empaquetado (*Channel Packed ORM: Occlusion / Roughness / Metallic*), conectar:
   - Canal Rojo (R) $\rightarrow$ `Ambient Occlusion`
   - Canal Verde (G) $\rightarrow$ `Roughness`
   - Canal Azul (B) $\rightarrow$ `Metallic`
4. Presionar **Save** y **Apply**.

![Grafo de conexiones de texturas dentro del Material Editor de Unreal Engine](images/3d_pipeline/20_ue5_material_editor_graph.jpg)

---

### 1.18 Comprobar el Modelo dentro del Escenario

1. Arrastrar el activo Static Mesh desde el Content Drawer hacia el nivel de juego.
2. Verificar la escala respecto a otros elementos y al tamaño del personaje.
3. Evaluar la interacción con la iluminación global (Lumen) desde múltiples ángulos de cámara.

![Modelos colocados e iluminados en el nivel de cafetería en Unreal Engine](images/3d_pipeline/21_ue5_level_scene_models.jpg)

---

## 2. Problema de Lectura Incorrecta de Texturas en Unreal

### Causa del Problema
Durante el desarrollo es común que un material visualizado en Substance Painter luzca plano, opaco o excesivamente reflectivo al llegar a Unreal Engine. Esto ocurre porque la plantilla de salida predeterminada de Substance (como *glTF PBR Metal Roughness* o formatos genéricos) canaliza los mapas de *Roughness* y *Metallic* en espacios de color y canales alfa incompatibles con los shaders de Unreal Engine.

```
Substance Painter (Plantilla Errónea)       Unreal Engine Material Shading
[ glTF Packing: G=Roughness, B=Metallic ] --X--> [ Desfase de brillo / Inversión de reflectividad ]

Substance Painter (Plantilla Unreal Engine) Unreal Engine Material Shading
[ Packed ORM: R=Occlusion, G=Roughness, B=Metallic ] ---> [ Lectura 100% precisa en Shaders PBR ]
```

---

### 2.1 Abrir la Ventana de Exportación
En Substance Painter, abrir el menú `File > Export Textures` (`Ctrl + Shift + E`).

---

### 2.2 Localizar la Plantilla de Exportación
En la parte superior de la ventana de configuración, hacer clic en el menú desplegable **Output Template**.

![Ventana de exportación mostrando la plantilla actual](images/3d_pipeline/22_substance_export_config.jpg)

---

### 2.3 Cambiar la Plantilla por una Compatible con Unreal Engine
1. En la lista de plantillas disponibles, seleccionar **Unreal Engine 4 / 5 (Packed)** o **PBR Metal Roughness (Unreal Format)**.
2. Esta plantilla empaqueta automáticamente:
   - `BaseColor` (sRGB)
   - `OcclusionRoughnessMetallic` (Linear / Non-sRGB empaquetado en canales R, G, B)
   - `Normal` (DirectX format con canal Y invertido para Unreal Engine)

![Selección de plantilla compatible en el menú Output Template](images/3d_pipeline/23_substance_output_template_dropdown.png)

---

### 2.4 Exportar Nuevamente las Texturas
1. Confirmar la ruta de destino.
2. Hacer clic en **Export**.
3. Las nuevas texturas quedarán guardadas con la estructura de canales optimizada.

---

### 2.5 Reemplazar las Texturas en Unreal Engine
1. En Unreal Engine, abrir la carpeta donde residen las texturas previas.
2. Presionar **Import** o arrastrar los nuevos archivos para sobreescribir los anteriores (*Reimport*).
3. Abrir el Material principal y reconectar los canales según la plantilla empaquetada.
4. Al guardar el material, el modelo reflejará inmediatamente el aspecto visual exacto concebido en Substance Painter.

> [!TIP]
> Si el mapa de normales genera sombras invertidas en Unreal (las hendiduras lucen como protuberancias), abre la textura en Unreal y en la sección *Texture* marca la casilla **Flip Green Channel**.

---

## 3. Corrección de Colisiones en Unreal

### ¿Qué es una Colisión?
Una **colisión** es una primitiva geométrica invisible y simplificada que el motor de física (Chaos Physics) utiliza para calcular impactos, bloqueos de personajes y detección de proyectiles o interacciones. Si un objeto carece de colisión o posee una colisión incorrecta, el personaje lo atravesará como un fantasma o quedará bloqueado en el aire antes de tocarlo.

---

### 3.1 Abrir el Modelo que Presenta el Problema
1. En el **Content Drawer**, localizar el Static Mesh afectado (ejemplo: `SM_Chair` o `SM_Table`).
2. Hacer doble clic sobre el activo para abrirlo en el **Static Mesh Editor**.

![Static Mesh Editor en Unreal Engine con el modelo cargado](images/3d_pipeline/24_ue5_static_mesh_editor_chair.jpg)

---

### 3.2 Mostrar la Colisión Actual
1. En la barra de herramientas superior del visor, presionar el menú **Show**.
2. Activar la opción **Simple Collision** (o atajo `Alt + C`).
3. Si el modelo posee colisión, aparecerán líneas verdes que delimitan la forma física actual.

![Visualización de colisión simple con líneas verdes en el Static Mesh Editor](images/3d_pipeline/25_ue5_show_simple_collision.jpg)

---

### 3.3 Eliminar una Colisión Incorrecta
Si la colisión existente encierra huecos transitables o bloquea zonas incorrectas:
1. Ir al menú superior: `Collision`.
2. Seleccionar la opción **Remove Collision**.
3. Las mallas de colisión previas se eliminarán al instante.

![Menú Collision con la opción Remove Collision resaltada](images/3d_pipeline/26_ue5_collision_remove_menu.jpg)

---

### 3.4 Crear una Colisión Sencilla
Para geometrías volumétricas simples (cajas, muros, postes):
1. En el menú `Collision`, seleccionar una primitiva básica:
   - **Add Box Simplified Collision:** Añade una caja delimitadora ajustable.
   - **Add Sphere Simplified Collision:** Añade una esfera.
   - **Add Capsule Simplified Collision:** Añade una cápsula.
2. Seleccionar las caras de la caja de colisión y usar las herramientas de traslación, rotación y escala para ajustarla al modelo.

![Añadir colisión simplificada de tipo caja desde el menú Collision](images/3d_pipeline/27_ue5_collision_add_box.jpg)

---

### 3.5 Utilizar Auto Convex Collision
Para modelos con formas orgánicas o huecos (como sillas con patas separadas o mostradores):
1. Ir al menú `Collision > Auto Convex Collision`.
2. En la esquina inferior derecha aparecerá el panel **Convex Decomposition**.

| Menú Auto Convex Collision | Panel de Parámetros Convex Decomposition |
| :---: | :---: |
| ![Opción Auto Convex Collision](images/3d_pipeline/28_ue5_auto_convex_collision_menu.jpg) | ![Panel de control Convex Decomposition](images/3d_pipeline/29_ue5_convex_decomposition_panel.jpg) |

---

### 3.6 Ajustar la Precisión de la Colisión
En el panel **Convex Decomposition**, configurar los siguientes parámetros:
* **Hull Count:** Cantidad máxima de formas convexas que generará el algoritmo (valores recomendados: `4` a `16`). A mayor valor, mejor adaptación a las patas y respaldos.
* **Max Hull Verts:** Límite de vértices por cada cápsula convexa (ejemplo: `16` a `32`).
* **Hull Precision:** Nivel de precisión del cálculo en vóxeles (ejemplo: `100,000`).
* Presionar el botón **Apply**. El algoritmo calculará y dibujará las cápsulas verdes adaptadas a la silueta.

---

### 3.7 Colisión Simple y Colisión Compleja
En el panel **Details** del Static Mesh Editor:
* Buscar la propiedad **Collision Complexity**:
  - `Default`: Utiliza colisión simple para movimiento y colisión compleja para trazas de línea (*line traces*).
  - `Use Simple Collision As Complex`: Fuerza el uso de las cápsulas simples en todas las consultas (óptimo para rendimiento).
  - `Use Complex Collision As Simple`: Utiliza la propia geometría poligonal del modelo como colisión física (reservado para escenarios estáticos de arquitectura compleja).

![Configuración de Collision Complexity en el panel Details](images/3d_pipeline/30_ue5_collision_complexity_settings.jpg)

---

### 3.8 Guardar los Cambios
Presionar el botón **Save** en el Static Mesh Editor y cerrar la ventana.

---

### 3.9 Probar la Colisión dentro del Juego
1. Colocar el modelo en el nivel.
2. Presionar **Play (PIE)** (`Alt + P`).
3. Acercar al personaje y caminar contra el objeto desde diversos ángulos, saltar sobre él y validar que no ocurra penetración de malla ni bloqueos invisibles alejados.

---

## 4. Creación de Materiales Luminosos y Cristalinos en Unreal

En esta sección se documenta la construcción desde cero de dos tipos de shaders especializados en el Material Editor de Unreal Engine 5.8: materiales de vidrio transparente translúcido y materiales autoiluminados con resplandor (*Emissive*).

---

### A. Creación de un Material Cristalino

Los materiales estándar en Unreal Engine son totalmente opacos. Para crear vidrios de ventanas, vitrinas o vasos, debemos habilitar el modelo de sombreado translúcido.

#### 4.1.1 Crear un Nuevo Material
1. En el Content Drawer, hacer clic derecho en un área vacía.
2. Seleccionar `Material`.
3. Nombrar el asset: `M_Cristal`.
4. Abrirlo haciendo doble clic.

![Creación de nuevo asset Material en el Content Drawer](images/3d_pipeline/31_ue5_create_new_material.jpg)

---

#### 4.1.2 Cambiar el Material para Permitir Transparencia
1. En el grafo del Material Editor, hacer clic en el nodo principal de resultados (`M_Cristal`).
2. En el panel **Details** de la izquierda, ubicar la sección **Material**:
3. Cambiar **Blend Mode** de `Opaque` a **`Translucent`**.
4. En **Lighting Mode** (en la sección *Translucency*), seleccionar **Surface TranslucencyVolume** o **Surface ForwardShading** para habilitar reflejos realistas sobre el cristal.

![Cambio de Blend Mode a Translucent en el panel Details](images/3d_pipeline/32_ue5_material_blend_mode_translucent.png)

---

#### 4.1.3 Configurar el Color del Cristal
1. Hacer clic derecho en el grafo y buscar **VectorParameter** (o mantener presionada la tecla `V` y hacer clic izquierdo).
2. Nombrar el parámetro: `Color_Cristal`.
3. Hacer doble clic en la muestra de color y seleccionar un tono de tinte (ejemplo: azul cian tenue, verde botella o blanco puro).
4. Conectar la salida del nodo al pin **Base Color** del nodo principal.

![Nodo VectorParameter conectado a Base Color](images/3d_pipeline/33_ue5_material_base_color_node.jpg)

---

#### 4.1.4 Configurar la Transparencia
1. Crear un nodo **ScalarParameter** (mantener pulsada la tecla `S` y hacer clic).
2. Nombrarlo `Opacity`.
3. Establecer su valor por defecto en un rango entre `0.1` y `0.4` (valores bajos confieren alta transparencia; valores cercanos a `1.0` confieren mayor opacidad).
4. Conectar la salida al pin **Opacity**.

![Nodo ScalarParameter de Opacity conectado al pin principal](images/3d_pipeline/34_ue5_material_opacity_roughness.jpg)

---

#### 4.1.5 Ajustar la Rugosidad del Cristal
1. Crear otro nodo **ScalarParameter** y nombrarlo `Roughness`.
2. Asignarle un valor bajo, entre `0.01` y `0.08`, para permitir reflejos nítidos y especulares.
3. Conectar la salida al pin **Roughness**.

![Grafo completo del shader de cristal en el Material Editor](images/3d_pipeline/35_ue5_material_glass_graph.jpg)

---

#### 4.1.6 Revisar el Material
1. Previsualizar la esfera en el visor del editor.
2. Presionar **Apply** y **Save**.
3. Arrastrar el material `M_Cristal` sobre las ventanas o vitrinas del nivel para comprobar la refracción y transparencia en tiempo real.

![Previsualización del material de cristal completado](images/3d_pipeline/36_ue5_material_glass_preview.jpg)

---

### B. Creación de un Material Luminoso

Los materiales emisivos simulan pantallas fluorescentes, botones indicadores, lámparas de neón o estufas encendidas.

#### 4.2.1 Crear el Material Luminoso
1. Crear un nuevo material en el Content Drawer (`M_Luz`).
2. Abrir el editor haciendo doble clic.

---

#### 4.2.2 Crear el Color de la Luz
1. Crear un nodo **VectorParameter** con el nombre `Color_Luz`.
2. Asignar el color deseado (verde neón, naranja fuego, cian eléctrico).

---

#### 4.2.3 Crear un Control para la Intensidad
Para que el material sobrepase el rango dinámico estándar y active el post-procesado de resplandor (*Bloom*):
1. Crear un **ScalarParameter** llamado `Intensidad` con un valor base de `5.0` a `20.0`.
2. Crear un nodo **Multiply** (mantener pulsada la tecla `M` y hacer clic).
3. Conectar `Color_Luz` a la entrada **A** de `Multiply`.
4. Conectar `Intensidad` a la entrada **B** de `Multiply`.
5. Conectar la salida de `Multiply` al pin **Emissive Color** del nodo principal de material.

![Grafo de nodos para material emisivo multiplicando color por intensidad](images/3d_pipeline/37_ue5_material_emissive_graph.jpg)

---

#### 4.2.4 Ajustar la Intensidad
* Valores entre `1.0` y `3.0`: Producen un color encendido básico.
* Valores entre `5.0` y `50.0`: Producen un resplandor volumétrico (*Bloom glow*) que tiñe los alrededores bajo Lumen.

---

#### 4.2.5 Aplicar el Material al Objeto
Presionar **Apply** y **Save**. Arrastrar el material sobre los paneles de luz de los electrodomésticos o mesas del restaurante.

![Material emisivo aplicado sobre el mostrador de comida en el nivel](images/3d_pipeline/38_ue5_emissive_material_applied.jpg)

---

#### 4.2.6 Importante: Material Luminoso no Siempre Significa una Luz Real

> [!IMPORTANT]
> **Diferencia entre Emissive y Light Components**
> 
> Aunque los materiales emisivos interactúan con el cálculo de radiosidad de Lumen en UE 5.8, no proyectan sombras directas de alta precisión para objetos dinámicos pequeños de la misma forma que una fuente de luz puntual (**Point Light**) o focal (**Spot Light**).
> 
> Para luminarias principales de una habitación, se debe colocar una **Point Light** o **Spot Light** complementando la geometría con shader emisivo.

---

## 5. Desarrollo de Animaciones con MetaHuman y Corrección de Huesos para Animaciones

Unreal Engine 5.8 introduce capacidades avanzadas para convertir grabaciones de video ordinarias en animaciones corporales 3D mediante **Markerless Motion Capture (MetaHuman Animator)**, eliminando la necesidad de trajes de captura (*MoCap*) o sensores ópticos.

```
+--------------------------+
|  Video de Cámara/Móvil   |  (Grabación de cuerpo completo y movimiento claro)
+------------+-------------+
             |
             v
+--------------------------+
|   Live Link Hub Ingest   |  (Mono Video Ingest -> Capture Data Asset)
+------------+-------------+
             |
             v
+--------------------------+
|  MetaHuman Performance   |  (Body Tracking -> Extracción de Skeleton MoCap)
+------------+-------------+
             |
             v
+--------------------------+
|  Exportación FBX a 3D    |  (Export Animation Sequence -> FBX)
+------------+-------------+
             |
             v
+--------------------------+
|   Retargeting Rokoko     |  (Mapeo de huesos MetaHuman -> Armature Low-Poly)
+------------+-------------+
             |
             v
+--------------------------+
|   Importación UE 5.8     |  (Animation Sequence final lista para Game/StateTree)
+--------------------------+
```

---

### 5.1 Grabar un Movimiento o Conseguir un Video
Para garantizar la máxima precisión en el seguimiento automático:
* **Encuadre completo:** El cuerpo entero (pies a cabeza) debe permanecer dentro del plano durante toda la acción.
* **Estabilidad:** Utilizar un trípode o base fija sin movimientos de cámara.
* **Iluminación homogénea:** Evitar sombras duras o contraluces.
* **Fondo despejado:** Minimizar objetos que obstruyan la silueta de los brazos y piernas.

---

### 5.2 Activar las Herramientas de MetaHuman
1. En Unreal Engine, ir a `Edit > Plugins`.
2. Buscar y habilitar:
   - **MetaHuman Animator**
   - **MetaHuman Animator Markerless Motion Capture** (Experimental)
3. Reiniciar el motor si lo solicita para cargar los módulos nativos.

| Plugin MetaHuman Markerless Motion Capture | Live Link Hub con Capture Manager |
| :---: | :---: |
| ![Plugin MetaHuman Markerless Motion Capture habilitado](images/3d_pipeline/39_ue5_plugin_metahuman_markerless.png) | ![Ventana Live Link Hub y Capture Manager](images/3d_pipeline/40_ue5_livelink_hub_capture_manager.png) |

---

### 5.3 Abrir Capture Manager
Ir a `Tools > Live Link Hub`. En la barra de herramientas, seleccionar **Capture Manager**.

---

### 5.4 Crear un Mono Video Ingest
1. Dentro de Capture Manager, ir al panel lateral **Data Devices**.
2. Presionar el botón **Add (+)**.
3. Seleccionar **Mono Video Ingest**.
4. En el campo **Source Path**, indicar la carpeta local donde se encuentra el archivo de video.

![Configuración del dispositivo Mono Video Ingest en Capture Manager](images/3d_pipeline/41_ue5_mono_video_ingest.png)

---

### 5.5 Importar el Video
1. Cuando Capture Manager liste el archivo de video detectado, seleccionarlo.
2. Presionar **Add to Queue**.
3. Verificar su aparición en la lista de trabajos (**Jobs List**).
4. Presionar **Start**.
5. Al finalizar el proceso, Unreal generará un activo **Capture Data Asset** con la información de frames lista para análisis.

![Lista de trabajos en Capture Manager procesando el video añadido](images/3d_pipeline/42_ue5_capture_manager_jobs_list.png)

---

### 5.6 Crear un MetaHuman Performance
1. En el Content Drawer, hacer clic derecho en la carpeta de animaciones.
2. Ir a `MetaHuman > MetaHuman Performance`.
3. Nombrar el activo (ejemplo: `MHP_Acrobacia` o `MHP_Walk`).
4. Abrirlo haciendo doble clic.

| Crear MetaHuman Performance | Ventana Principal de MetaHuman Performance |
| :---: | :---: |
| ![Creación de MetaHuman Performance en el menú contextual](images/3d_pipeline/43_ue5_create_metahuman_performance.jpg) | ![Ventana de configuración MetaHuman Performance](images/3d_pipeline/44_ue5_metahuman_performance_window.jpg) |

---

### 5.7 Asignar la Captura
En el panel **Details** de la ventana de rendimiento:
1. Localizar el campo **Footage Capture Data**.
2. Asignar el activo de captura generado previamente en el paso 5.5.

---

### 5.8 Activar Body Tracking
En los parámetros de procesamiento:
1. Marcar la casilla **Body Tracking** para enfocar el análisis en los miembros corporales.
2. Desactivar el rastreo facial si el video de referencia solo se utilizará para locomoción y acrobacias de cuerpo completo.

![Casilla Body Tracking activada en los parámetros de procesamiento](images/3d_pipeline/45_ue5_metahuman_body_tracking_enabled.jpg)

---

### 5.9 Seleccionar el Fragmento del Video
En la línea de tiempo inferior:
1. Arrastrar los delimitadores de inicio y final para seleccionar únicamente el rango útil donde se ejecuta el movimiento.
2. Esto optimiza el tiempo de cálculo y elimina transiciones estáticas antes o después de la acción.

| Selección de Rango en Línea de Tiempo | Animación Corporal Procesada y Malla Esquelética |
| :---: | :---: |
| ![Selección de rango en timeline](images/3d_pipeline/46_ue5_metahuman_timeline_selection.jpg) | ![Visualización del movimiento corporal procesado](images/3d_pipeline/47_ue5_metahuman_processed_animation.jpg) |

---

### 5.10 Procesar la Animación
1. Presionar el botón **Process** en la barra superior.
2. MetaHuman Animator ejecutará el solver de visión computacional, resolviendo la traslación y rotación de todas las articulaciones.

---

### Transferir la Animación al Personaje Low-Poly

Dado que el esqueleto resultante de MetaHuman posee una jerarquía compleja con decenas de huesos faciales y torsionales, y nuestro personaje estilizado (*Low-Poly*) utiliza una estructura simplificada, transferimos el movimiento mediante **Retargeting en Blender**.

---

### 5.11 Exportar la Animación Obtenida
1. En MetaHuman Performance, presionar **Export Animation**.
2. Guardar la secuencia generada en el Content Drawer.

![Diálogo Export Animation Sequence en Unreal Engine](images/3d_pipeline/48_ue5_export_animation_dialog.jpg)

---

### 5.12 Exportar la Animación como FBX
1. Localizar el activo de animación en el Content Drawer.
2. Hacer clic derecho: `Asset Actions > Export...`.
3. Guardar el archivo con extensión `.fbx`.

| Exportación FBX desde Unreal Engine | Personaje Low-Poly con su Armature en Blender |
| :---: | :---: |
| ![Asset Actions Export FBX en Unreal](images/3d_pipeline/49_ue5_asset_actions_export_fbx.jpg) | ![Personaje low-poly y armature en Blender](images/3d_pipeline/50_blender_character_armature.jpg) |

---

### 5.13 Abrir Blender
Abrir la escena de Blender donde reside el personaje estilizado con su Armature base configurado.

---

### 5.14 Importar la Animación de MetaHuman
1. Ir a `File > Import > FBX`.
2. Seleccionar la animación exportada desde Unreal Engine.
3. Ahora la escena contendrá ambos esqueletos: el esqueleto animado de MetaHuman y el esqueleto destino del personaje.

![Ambos esqueletos importados en el visor 3D de Blender](images/3d_pipeline/51_blender_both_skeletons_imported.jpg)

---

### 5.15 Instalar y Activar Rokoko
Utilizamos el add-on **Rokoko Studio Live for Blender** para realizar el retargeting automático de huesos.
1. Instalar el complemento desde las preferencias de Blender.
2. Verificar que la pestaña **Rokoko** aparezca en la barra lateral derecha (*N-Panel*).

![Panel del add-on Rokoko en la barra lateral de Blender](images/3d_pipeline/52_blender_rokoko_panel.jpg)

---

### 5.16 Abrir la Herramienta Retargeting
En el panel de Rokoko:
1. Desplegar la sección **Retargeting**.
2. En el campo **Source**, seleccionar el esqueleto de MetaHuman con la animación.
3. En el campo **Target**, seleccionar el esqueleto del personaje propio (*Armature*).

![Selección de armatures Source y Target en Rokoko](images/3d_pipeline/53_blender_rokoko_source_target.jpg)

---

### 5.17 Crear el Bone List
Hacer clic en el botón **Rebuild Bone List** / **Build Bone List**. Rokoko analizará y emparejará automáticamente los nombres de articulaciones coincidentes (`hips`, `spine`, `upperarm`, `thigh`, etc.).

![Lista de correspondencia de huesos generada por Rokoko](images/3d_pipeline/54_blender_rokoko_bone_list.png)

---

### 5.18 Revisar Manualmente los Huesos
Dado que el personaje low-poly posee menos huesos, revisar las asignaciones:
* Omitir huesos auxiliares (*twist bones*) o falanges extra que no existan en el modelo destino.
* Confirmar que extremidades simétricas estén correctamente asignadas (`_l` con `_l`, `_r` con `_r`).

---

### 5.19 Comprobar las Poses de Ambos Esqueletos
Antes de ejecutar el cálculo, asegurar que ambos esqueletos coincidan en su postura de reposo (*T-Pose* o *A-Pose*). Si un esqueleto tiene los brazos en posición horizontal y el otro a \(45^\circ\), alinear la pose en **Pose Mode** para evitar torsiones no deseadas.

---

### 5.20 Ejecutar el Retargeting
Hacer clic en el botón **Retarget Animation**. Rokoko transferirá las curvas de animación y rotaciones frame a frame hacia el esqueleto del personaje low-poly.

![Botón Retarget Animation en el panel de Rokoko](images/3d_pipeline/55_blender_rokoko_retarget_button.jpg)

---

### 5.21 Reproducir la Animación
Presionar la barra espaciadora (*Play*) en la línea de tiempo de Blender para comprobar que el personaje reproduzca fluidamente el movimiento capturado.

![Personaje ejecutando la animación retargeteada en Blender](images/3d_pipeline/56_blender_character_animation_playback.jpg)

---

### 5.22 Preparar el Personaje para Exportar
Comprobar que no existan capas de animación duplicadas en el *NLA Editor* o el *Action Editor* y que los modificadores de Armature permanezcan intactos.

---

### 5.23 Exportar desde Blender como FBX
1. Seleccionar la malla del personaje y su Armature.
2. Ir a `File > Export > FBX (.fbx)`.
3. Activar **Selected Objects**.
4. En **Object Types**, seleccionar **Armature** y **Mesh**.
5. En la pestaña **Bake Animation**, marcar la casilla para exportar los *keyframes*.

![Configuración de exportación FBX con animación horneada desde Blender](images/3d_pipeline/57_blender_export_animated_character.png)

---

### 5.24 Importar Nuevamente en Unreal Engine
1. En Unreal Engine, abrir el Content Drawer.
2. Presionar **Import** y seleccionar el archivo FBX generado.
3. En la ventana de importación:
   - Si es la primera vez que se importa el personaje, marcar **Import Mesh**, **Import Animations** y dejar que Unreal genere un **Skeleton**.

![Diálogo de importación FBX para Skeletal Mesh y animación](images/3d_pipeline/58_ue5_fbx_import_skeletal_mesh.png)

---

### 5.25 Si el Personaje ya Existe en Unreal
Si el Skeletal Mesh del personaje ya fue importado con anterioridad, simplemente seleccionar en el campo **Skeleton** el esqueleto existente del proyecto. De esta forma, Unreal importará la animación como un nuevo activo **Animation Sequence** compatible con todos los personajes que compartan dicho esqueleto.

---

### 5.26 Probar la Animación en Unreal
Abrir el activo **Animation Sequence** haciendo doble clic. Validar:
* Traslación de caderas y pies (*Root Motion / in-place*).
* Orientación frontal correcta.
* Sin distorsiones en hombros o rodillas.

![Animation Sequence reproduciéndose en el editor de animaciones de UE5](images/3d_pipeline/59_ue5_animation_sequence_editor.jpg)

---

### 5.27 Utilizarla dentro del Proyecto
La secuencia de animación está lista para incorporarse en:
* **Animation Blueprints (AnimBP):** Máquinas de estados de locomoción y saltos.
* **StateTree AI Tasks:** Tareas de comportamiento de clientes y meseros (`GSStateTreeTask_PlayAnimation`).
* **Level Sequencer:** Cinemáticas y eventos coreografiados de gameplay.

![Personaje ejecutando la animación directamente en el nivel de juego](images/3d_pipeline/60_ue5_character_in_gameplay_scene.jpg)

---

## 6. Guía de Asignación de Pesos

La asignación de pesos (**Weight Painting**) define qué proporción de influencia ejerce cada hueso del esqueleto sobre cada vértice de la malla tridimensional. Una calibración correcta garantiza deformaciones naturales y previene que partes sólidas del cuerpo se estiren o flexionen erróneamente.

---

### 6.1 Comprobar que el Personaje Tenga un Esqueleto
El personaje debe contar con una malla limpia y un Armature cuyos huesos coincidan exactamente con la posición anatómica de las articulaciones.

![Malla y esqueleto alineados en vista ortográfica frontal](images/3d_pipeline/61_blender_character_front_armature.png)

---

### 6.2 Vincular la Malla al Esqueleto
1. En **Object Mode**, seleccionar primero la malla del personaje.
2. Manteniendo presionada la tecla `Shift`, seleccionar el Armature (el esqueleto debe quedar con el contorno amarillo activo).
3. Presionar el atajo: `Ctrl + P`.
4. En el menú emergente **Set Parent To**, seleccionar **With Automatic Weights**.
5. Blender calculará las influencias por proximidad y creará los grupos de vértices (**Vertex Groups**) correspondientes.

![Menú Set Parent To seleccionando With Automatic Weights](images/3d_pipeline/62_blender_parent_automatic_weights.png)

---

### 6.3 Probar la Deformación
1. Seleccionar únicamente el Armature y cambiar a **Pose Mode** (`Ctrl + Tab`).
2. Seleccionar huesos individuales (ejemplo: brazo, pierna, cabeza) y rotarlos con la tecla `R`.
3. Identificar si al girar el brazo se deforma indebidamente la espalda o el abdomen.

![Comprobación de flexión en Pose Mode para detectar problemas](images/3d_pipeline/63_blender_weight_paint_mode.jpg)

---

### 6.4 Entrar en Weight Paint
1. En **Object Mode**, seleccionar el Armature, luego con `Shift` seleccionar la malla.
2. Cambiar al modo **Weight Paint** (`Ctrl + Tab` $\rightarrow$ *Weight Paint*).
3. La malla se coloreará mediante un mapa de calor que representa el grado de influencia:
   - 🔴 **Rojo (\(1.0\)):** Influencia total (el vértice sigue el movimiento al 100%).
   - 🟡 **Amarillo / Verde (\(0.5\)):** Influencia media (transición suave en articulaciones).
   - 🔵 **Azul (\(0.0\)):** Nula influencia (el vértice no se mueve con este hueso).

---

### 6.5 Seleccionar el Hueso que se Quiere Corregir
Hacer `Ctrl + Clic Izquierdo` sobre el hueso deseado. El visor actualizará instantáneamente el mapa de calor mostrando el peso de dicho hueso sobre el modelo.

| Selección de Hueso en Weight Paint | Gradiente de Pesos en Articulación |
| :---: | :---: |
| ![Selección de hueso en Weight Paint](images/3d_pipeline/64_blender_weight_paint_bone_select.png) | ![Mapa de calor de pesos en pierna y rodilla](images/3d_pipeline/65_blender_bone_weight_gradient.jpg) |

---

### 6.6 Añadir o Quitar Influencia
En la barra de herramientas lateral izquierda, seleccionar el pincel requerido:
* **Draw (Add):** Añade peso a la zona pintada (útil para rigidizar zonas del miembro).
* **Subtract:** Elimina peso no deseado en áreas adyacentes.
* **Blur:** Difumina y suaviza la frontera entre pesos, eliminando arrugas bruscas.
* **Average:** Iguala los valores en torno al promedio del área.

![Herramientas y pinceles disponibles en Weight Paint](images/3d_pipeline/66_blender_weight_paint_tools.png)

---

### 6.7 Corregir Zonas que se Mueven con el Hueso Equivocado
Si al mover el hueso del brazo derecho el torso se estira:
1. Seleccionar el hueso del brazo (`Ctrl + Clic`).
2. Usar el pincel **Subtract** con *Weight = 0.0* y *Strength = 1.0* para pintar sobre el torso hasta que quede azul oscuro puro.
3. Seleccionar el hueso del torso (`Spine` o `Chest`).
4. Usar el pincel **Draw** (*Add*) para devolver el control al torso en esa zona.

---

### 6.8 Normalizar los Pesos
Para evitar que un vértice sume más del \(100\%\) (\(1.0\)) entre múltiples huesos o quede huérfano:
1. En el menú superior de Weight Paint, ir a `Weights > Normalize All`.
2. Marcar la opción *Lock Active* desmarcada para recalcular todos los grupos.
3. Ir a `Weights > Clean` para purgar influencias residuales insignificantes menores a \(0.01\).

![Opciones Normalize All y Clean en el menú Weights](images/3d_pipeline/67_blender_weights_normalize_clean.jpg)

---

### 6.9 Revisar Articulaciones Importantes
Prestar especial atención a las zonas de flexión continua:
* **Hombros y Axilas:** Mantener una transición gradual con el pecho para evitar que colapse al levantar los brazos.
* **Codos y Rodillas:** Conservar suficiente rigidez en la parte frontal para mantener el volumen anatómico.
* **Muñecas y Tobillos:** Evitar torsiones de caramelo (*candy-wrapper effect*).
* **Cuello y Mandíbula:** Asegurar que la base de la cabeza permanezca anclada al cuello.

---

### 6.10 Probar los Pesos con la Animación
1. Cambiar a la línea de tiempo y presionar **Play**.
2. Mientras la animación corre en bucle, observar el comportamiento de las mallas desde todos los ángulos de cámara.

---

### 6.11 Repetir las Correcciones Necesarias
El proceso de pesado sigue un ciclo de refinamiento iterativo:

$$\text{Reproducir Animación} \longrightarrow \text{Detectar Deformación} \longrightarrow \text{Pintar/Ajustar Pesos} \longrightarrow \text{Normalizar} \longrightarrow \text{Revalidar}$$

---

## 7. Cómo Limpiar y Preparar Modelos para Exportación a Unreal

Antes de exportar cualquier activo estático o animado hacia Unreal Engine 5.8, se debe aplicar la siguiente lista de verificación técnica de control de calidad (*QA Checklist*).

---

### 7.1 Revisar y Organizar el Modelo
1. Eliminar objetos auxiliares no utilizados (cámaras de referencia, luces de prueba, curvas guía, planos de fondo).
2. Asignar nombres limpios y estandarizados en el Outliner:
   - ✅ `SM_Mesa_Cafeteria`, `SM_Silla_Madera`, `SM_Oven_Station`
   - ❌ `Cube.001`, `Cylinder.045`, `Plane`

---

### 7.2 Limpiar la Geometría
1. Seleccionar la malla y entrar en **Edit Mode** (`Tab`).
2. Seleccionar todos los vértices presionando la tecla `A`.
3. Presionar la tecla `M` y seleccionar **By Distance** (*Merge by Distance*).
4. Blender unificará vértices duplicados superpuestos en la misma coordenada espacial.

---

### 7.3 Revisar las Normales
1. En el menú de superposiciones del visor (**Viewport Overlays**), activar la casilla **Face Orientation**.
2. Todo el exterior de la malla debe mostrarse de color **Azul**.
3. Si alguna cara aparece en color **Rojo** (normal invertida):
   - Seleccionar toda la malla con `A`.
   - Presionar `Shift + N` (*Recalculate Outside*).

---

### 7.4 Revisar Errores Non-Manifold
Para evitar agujeros invisibles o caras de grosor cero que rompan Nanite y Chaos Physics:
1. En Edit Mode, ir a `Select > Select All by Trait > Non-Manifold`.
2. Corregir cualquier borde suelto o cara interna seleccionada.

---

### 7.5 Aplicar las Transformaciones
Las transformaciones no aplicadas generan escalas distorsionadas en Unreal:
1. En **Object Mode**, seleccionar el objeto.
2. Presionar `Ctrl + A`.
3. Seleccionar **Rotation & Scale**.
4. Confirmar en el panel de transformaciones que:
   $$\text{Rotation} = (0^\circ, 0^\circ, 0^\circ), \quad \text{Scale} = (1.0, 1.0, 1.0)$$

---

### 7.6 Revisar Tamaño, Posición y Origen
1. Centrar el origen del objeto: Clic derecho $\rightarrow$ `Set Origin > Origin to Geometry`.
2. Para objetos que reposan sobre el suelo o estaciones de trabajo, colocar el punto de pivote en la base inferior (\(Z = 0\)).

---

### 7.7 Revisar el Mapa UV
1. Abrir el espacio **UV Editing**.
2. Confirmar que no existan caras con área UV igual a cero o estiramientos severos.
3. Asegurar que las islas UV posean un margen de separación (*padding*) adecuado (mínimo 4 a 8 píxeles en resolución 2K) para evitar sangrado de textura (*texture bleeding*).

---

### 7.8 Revisar los Materiales
1. Eliminar slots de materiales vacíos o no utilizados en la pestaña de propiedades de material.
2. Renombrar los materiales con el prefijo estándar: `M_Madera_01`, `M_Metal_Inox`, `M_Cristal`.

---

### 7.9 Revisar los Modificadores
1. Si se utilizaron modificadores como *Mirror*, *Solidify*, *Bevel* o *Subdivision Surface*, verificar que el resultado sea óptimo.
2. Si el objeto se exporta como Static Mesh, aplicar los modificadores definitivamente en la pestaña de modificadores (`Modifiers > Apply` o `Ctrl + A` sobre el modificador).

---

### 7.10 Unir Piezas Cuando Sea Necesario
1. Si un objeto está compuesto por múltiples piezas independientes que compartirán el mismo actor estático, seleccionarlas todas en Object Mode.
2. Presionar `Ctrl + J` (*Join*) para fusionarlas en una única malla optimizada.

---

### 7.11 Seleccionar el Modelo para Exportar
Seleccionar únicamente la malla a exportar para evitar incluir elementos fuera de contexto.

---

### 7.12 Exportar a FBX
1. `File > Export > FBX (.fbx)`.
2. Activar casilla **Selected Objects**.
3. Definir nombre estandarizado (ejemplo: `SM_Mesa_Cafeteria_Final.fbx`).
4. Presionar **Export FBX**.

---

### 7.13 Importar y Comprobar en Unreal Engine
1. Arrastrar el archivo FBX al Content Drawer de Unreal Engine 5.8.
2. Colocar el modelo en el nivel y realizar la comprobación final de 6 puntos:
   - [x] **Escala:** Tamaño correcto en comparación con el maniquí del jugador.
   - [x] **Rotación:** Eje frontal apuntando hacia la orientación deseada.
   - [x] **Geometría:** Sin polígonos faltantes ni sombreados rotos.
   - [x] **Normales e Iluminación:** Interacción coherente con luces dinámicas y Lumen.
   - [x] **Materiales y Texturas:** Asignación correcta de mapas PBR y shaders.
   - [x] **Colisión Física:** Primitivas ajustadas para interacción perfecta con el personaje.

---

## Resumen de Atajos y Referencias Rápidas

| Herramienta | Atajo de Teclado / Acción | Propósito |
| :--- | :--- | :--- |
| **Blender** | `Ctrl + Shift + T` | Configuración automática de nodos PBR (Node Wrangler) |
| **Blender** | `Shift + N` | Recalcular normales hacia el exterior |
| **Blender** | `Ctrl + A` $\rightarrow$ *Rotation & Scale* | Aplicar transformaciones al objeto |
| **Blender** | `M` $\rightarrow$ *By Distance* | Fusionar vértices duplicados |
| **Blender** | `Ctrl + P` $\rightarrow$ *With Automatic Weights* | Emparentar malla a esqueleto con pesos automáticos |
| **Blender** | `Ctrl + Tab` | Alternar entre Object Mode, Pose Mode y Weight Paint |
| **Substance Painter** | `Ctrl + Shift + E` | Abrir menú de exportación de texturas |
| **Unreal Engine** | `Alt + C` | Alternar visualización de colisión simple en Static Mesh Editor |
| **Unreal Engine** | `Ctrl + Space` | Abrir / Cerrar Content Drawer |
| **Unreal Engine** | `Alt + P` | Iniciar Play in Editor (PIE) para probar físicas e interacciones |

---

*ProjectF (Good Service) – 3D Pipeline & Asset Creation Guide – Unreal Engine 5.8*
