provider "google" {
  credentials = file("account.json")
  project = "token"
  region      = "us-central1"
  zone        = "us-central1-a"
}

resource "google_compute_instance" "default" {
  name = "ledger-node-1"
  machine_type = "n1-standard-1"
  zone = "us-central1-a"

  tags = [
    "ledger"
  ]

  boot_disk {
    initialize_params {
      image = "debian-cloud/debian-9"
    }
  }

  scratch_disk {}

  network_interface {
    network = "default"
    access_config {}
  }
}