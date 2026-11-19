const menu = document.querySelector(".mobile-menu");

if (menu) {
  menu.addEventListener("click", (event) => {
    if (event.target.closest("a")) {
      menu.open = false;
    }
  });

  menu.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && menu.open) {
      menu.open = false;
      menu.querySelector("summary").focus();
    }
  });
}
