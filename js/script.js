const url = 'http://*.*.*.*';

function showAddForm() {
  const table = document.getElementById('data-table');
  if (!table) {
    alert('Сначала загрузите таблицу.');
    return;
  }
  const headers = table.querySelectorAll('th');
  const inputFields = document.getElementById('inputFields');
  inputFields.innerHTML = ''; // Очищаем предыдущие поля ввода
  headers.forEach(header => {
    if (header.textContent !== 'Действия') { // Пропускаем колонку действий
      const inputDiv = document.createElement('div');
      const inputLabel = document.createElement('label');
      inputLabel.textContent = header.textContent + ': ';
      const input = document.createElement('input');
      input.type = 'text';
      input.name = header.textContent;
      inputDiv.appendChild(inputLabel);
      inputDiv.appendChild(input);
      inputFields.appendChild(inputDiv);
    }
  });
  document.getElementById('addForm').style.display = 'block'; // Показываем форму
}


function addNewRecord() {
  const inputs = document.getElementById('inputFields').querySelectorAll('input');
  const data = {};
  inputs.forEach(input => {
    data[input.name] = input.value;
  });

  fetch(url + `/add/${currentTable}`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(data)
  })
  .then(response => {
    if (response.ok) {
      alert('Запись успешно добавлена!');
      document.getElementById('addForm').style.display = 'none'; // Скрываем форму
      loadData(currentTable); // Обновляем данные таблицы
    } else {
      response.json().then(data => alert('Ошибка добавления: ' + data.error));
    }
  })
  .catch(error => alert('Ошибка при добавлении записи: ' + error));
}

function loadTables() {
  fetch(url + '/getTables')
    .then(response => response.json())
    .then(data => {
      const tables = data.body;
      const tableSelect = document.createElement('select');
      tableSelect.style.display = "inline-block";
      tableSelect.onchange = () => {
        currentTable = tableSelect.value;
        loadData(tableSelect.value);
      };
      tables.forEach(table => {
        const option = new Option(table.table_name, table.table_name);
        tableSelect.appendChild(option);
      });
      document.getElementById('table-container').innerHTML = '';
      document.getElementById('topPanel').appendChild(tableSelect);
      currentTable = tables[0].table_name;
      loadData(tables[0].table_name); // Load first table by default
      document.getElementById('saveBtn').style.display = 'inline-block';
    });
}

function loadData(tableName) {
  fetch(url + `/data/${tableName}`)
    .then(response => response.json())
    .then(data => displayData(data));
}

function searchByID() {
  const id = document.getElementById('searchID').value;
  if (id) {
    fetch(url + `/findById/${currentTable}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ id }),
    })
      .then(response => response.json())
      .then(data => displayData(data))
      .catch(error => alert('Ошибка при поиске данных: ' + error));
  } else {
    alert('Пожалуйста, введите ID для поиска.');
  }
}

function displayData(data) {
  if (data.body.length === 0) {
    alert('Нет данных для отображения.');
    return;
  }

  const fields = data.body[0];
  const table = document.createElement('table');
  table.setAttribute('id', 'data-table');
  
  // Создаем заголовок таблицы
  const headerRow = document.createElement('tr');
  Object.keys(fields).forEach(field => {
    const header = document.createElement('th');
    header.textContent = field;
    headerRow.appendChild(header);
  });
  table.appendChild(headerRow);

  // Заполняем таблицу данными
  data.body.forEach(row => {
    const rowElement = document.createElement('tr');
    Object.entries(row).forEach(([field, value]) => {
      const cell = document.createElement('td');
      const input = document.createElement('input');
      input.value = value;
      input.setAttribute('name', field);
      cell.appendChild(input);
      rowElement.appendChild(cell);
    });
    table.appendChild(rowElement);
  });

  const container = document.getElementById('table-container');
  container.innerHTML = ''; // Очищаем предыдущие данные
  container.appendChild(table); // Добавляем новую таблицу с данными
}


function deleteByID() {
    const id = document.getElementById('searchID').value;
    if (id) {
      const deleteBtn = document.getElementById('deleteByIDBtn');
      deleteBtn.disabled = true;
      deleteBtn.classList.add('loading'); 
  
      fetch(url + `/delete/${currentTable}`, {
        method: 'DELETE',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ id }),
      })
        .then(response => {
          deleteBtn.disabled = false;
          deleteBtn.classList.remove('loading'); 
  
          if (response.ok) {
            alert('Запись успешно удалена!');
            loadData(currentTable); 
          } else {
            alert('Ошибка при удалении: ' + response.statusText);
          }
        })
        .catch(error => {
          deleteBtn.disabled = false; 
          deleteBtn.classList.remove('loading'); 
          alert('Ошибка при удалении: ' + error);
        });
    } else {
      alert('Пожалуйста, введите ID для удаления.');
    }
  }

function saveChanges() {
  const table = document.getElementById('data-table');
  const rows = table.getElementsByTagName('tr');
  const updates = [];

  for (let i = 1; i < rows.length; i++) {
    const cells = rows[i].getElementsByTagName('input');
    const updateData = {};
    for (let cell of cells) {
      updateData[cell.getAttribute('name')] = cell.value;
    }
    updates.push(updateData);
  }

  const saveBtn = document.getElementById('saveBtn');
  saveBtn.disabled = true; 
  saveBtn.classList.add('loading'); 

  updates.forEach(update =>
      fetch(url + `/update/${currentTable}`, {
      method: 'POST',
      headers: {
          'Content-Type': 'application/json',
      },
      body: JSON.stringify(update),
      })
      .then(response => {
          saveBtn.disabled = false; 
          saveBtn.classList.remove('loading'); 
  
          if (response.ok) {
          alert('Данные успешно сохранены!');
          } else {
          alert('Ошибка при сохранении данных: ' + response.statusText);
          }
      })
      .catch(error => {
          saveBtn.disabled = false; 
          saveBtn.classList.remove('loading'); 
          alert('Ошибка при сохранении данных: ' + error);
      }));
}

