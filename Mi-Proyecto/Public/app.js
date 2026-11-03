'use strict';

const apiResultado   = document.querySelector('#api-resultado');
const formAlumno     = document.querySelector('#form-alumno');
const btnRegistrar   = document.querySelector('#btn-registrar');
const btnMensaje     = document.querySelector('#btn-mensaje');
const btnPromedio    = document.querySelector('#btn-promedio');
const btnLimpiar     = document.querySelector('#btn-limpiar');
const resultado      = document.querySelector('#resultado');

const tbody          = document.querySelector('#tabla-alumnos tbody');
const tablaEstado    = document.querySelector('#tabla-estado');
const btnActualizar  = document.querySelector('#btn-actualizar');

if (formAlumno) {
  formAlumno.addEventListener('submit', async (e) => {
    e.preventDefault();

    const datos = {
      nombre:    document.querySelector('#nombre').value.trim(),
      edad:      Number(document.querySelector('#edad').value),
      matricula: document.querySelector('#matricula').value.trim(),
      correo:    document.querySelector('#correo').value.trim() 
    };

    if (!datos.nombre || !datos.edad || !datos.matricula || !datos.correo) {
      apiResultado.textContent = 'Completa todos los campos antes de enviar.';
      apiResultado.classList.add('error');
      return;
    }

    apiResultado.textContent = 'Enviando datos al servidor...';
    apiResultado.classList.remove('error');
    if (btnRegistrar) btnRegistrar.disabled = true;

    try {
      const res = await fetch('/api/alumnos', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(datos)
      });

      if (!res.ok) {
        if (res.status === 409) throw new Error('La matrícula ya está registrada.');
        throw new Error(`Error HTTP ${res.status}`);
      }

      const texto = await res.text();
      apiResultado.textContent = `Servidor respondió: ${texto}`;
      formAlumno.reset();
      
      await cargarAlumnos();

    } catch (err) {
      apiResultado.textContent = `Error al enviar: ${err.message}`;
      apiResultado.classList.add('error');
    } finally {
      if (btnRegistrar) btnRegistrar.disabled = false;
    }
  });
}

const botonesApi = document.querySelectorAll('.btn-api');

async function consultarAPI(endpoint, boton) {
  if(apiResultado) {
    apiResultado.textContent = 'Cargando...';
    apiResultado.classList.remove('error');
  }
  if (boton) boton.disabled = true;

  try {
    const res = await fetch(endpoint);
    if (!res.ok) throw new Error(`HTTP Error ${res.status}`);
    const texto = await res.text();
    if(apiResultado) apiResultado.textContent = texto;
  } catch (err) {
    if(apiResultado) {
      apiResultado.textContent = `Error: ${err.message}`;
      apiResultado.classList.add('error');
    }
  } finally {
    if (boton) boton.disabled = false;
  }
}

botonesApi.forEach((boton) => {
  boton.addEventListener('click', () => {
    consultarAPI(boton.dataset.endpoint, boton);
  });
});

let intervaloReloj = null;
const btnReloj = document.querySelector('#btn-reloj');

if (btnReloj) {
  btnReloj.addEventListener('click', () => {
    if (intervaloReloj) {
      clearInterval(intervaloReloj);
      intervaloReloj = null;
      btnReloj.textContent = 'Iniciar Reloj (5s)';
      btnReloj.classList.replace('btn-peligro', 'btn-editar');
      if(apiResultado) apiResultado.textContent = 'Reloj detenido.';
    } else {
      btnReloj.textContent = 'Detener Reloj';
      btnReloj.classList.replace('btn-editar', 'btn-peligro');
      consultarAPI('/api/hora', btnReloj);
      intervaloReloj = setInterval(() => {
        consultarAPI('/api/hora', btnReloj);
      }, 5000);
    }
  });
}

async function cargarAlumnos() {
  if (!tbody) return;
  
  if (tablaEstado) tablaEstado.textContent = 'Cargando...';
  if (btnActualizar) btnActualizar.disabled = true;

  try {
    const res = await fetch('/api/alumnos');
    if (!res.ok) throw new Error(`HTTP ${res.status}`);

    const lista = await res.json();
    renderizarTabla(lista);
    actualizarResumenDesdeServidor(lista);

    if (tablaEstado) tablaEstado.textContent = `${lista.length} alumno(s) en la base de datos.`;
  } catch (err) {
    if (tablaEstado) tablaEstado.textContent = `Error al cargar: ${err.message}`;
    console.error(err);
  } finally {
    if (btnActualizar) btnActualizar.disabled = false;
  }
}

function renderizarTabla(lista) {
  if (!tbody) return;
  tbody.innerHTML = '';

  if (lista.length === 0) {
    const fila = document.createElement('tr');
    const td   = document.createElement('td');
    td.colSpan = 3;
    td.textContent = 'No hay alumnos registrados en el servidor.';
    td.className = 'vacio';
    fila.appendChild(td);
    tbody.appendChild(fila);
    return;
  }

  lista.forEach((a) => {
    const tr = document.createElement('tr');

    const tdMat  = document.createElement('td');
    tdMat.textContent  = a.matricula;
    const tdNom  = document.createElement('td');
    tdNom.textContent  = a.nombre;
    const tdEdad = document.createElement('td');
    tdEdad.textContent = a.edad;

    tr.appendChild(tdMat);
    tr.appendChild(tdNom);
    tr.appendChild(tdEdad);
    tbody.appendChild(tr);
  });
}

function actualizarResumenDesdeServidor(lista) {
  const total    = document.querySelector('#total-alumnos');
  const promedio = document.querySelector('#promedio-general');
  const ultimo   = document.querySelector('#ultimo-alumno');
  
  if (!total || !promedio || !ultimo) return;

  total.textContent = lista.length;

  if (lista.length === 0) {
    promedio.textContent = '—';
    ultimo.textContent   = '—';
    return;
  }

  const sumaEdades = lista.reduce((acc, a) => acc + Number(a.edad), 0);
  promedio.textContent = (sumaEdades / lista.length).toFixed(1);
  ultimo.textContent   = lista[lista.length - 1].nombre;
}

if (btnActualizar) {
  btnActualizar.addEventListener('click', cargarAlumnos);
}
document.addEventListener('DOMContentLoaded', cargarAlumnos);