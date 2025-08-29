function performSearch() {
  const query = document.getElementById("query").value;
  fetch(`/search?query=${encodeURIComponent(query)}`)
    .then(response => response.json())
    .then(data => {
      const resultsList = document.getElementById("results");
      const backBtn = document.getElementById("back-btn");
      resultsList.innerHTML = "";

      if (data.length === 0) {
        resultsList.innerHTML = "<li>No results found</li>";
      } else {
        backBtn.style.display = "inline-block";
        data.forEach(item => {
          const li = document.createElement("li");
          li.innerHTML = `
            <h2>${item.keyword}</h2>
            <a href="${item.pdf}" target="_blank">📄 PDF</a>
            <a href="${item.gfg}" target="_blank">📘 GeeksforGeeks</a>
            <a href="${item.wiki}" target="_blank">🌐 Wikipedia</a>
            <h3>🎥 YouTube Preview:</h3>
            <iframe width="400" height="225" src="${item.yt}" frameborder="0" allowfullscreen></iframe>
          `;
          resultsList.appendChild(li);
        });
      }
    })
    .catch(error => console.error("Error:", error));
}

function goBack() {
  document.getElementById("results").innerHTML = "";
  document.getElementById("back-btn").style.display = "none";
}
